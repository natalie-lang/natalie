module Natalie
  class Compiler
    class MacroExpander
      def initialize(ast:, path:, load_path:, interpret:, compiler_context:, log_load_error: false)
        @ast = ast
        @path = path
        @load_path = load_path
        @interpret = interpret
        @compiler_context = compiler_context
        @log_load_error = log_load_error
        @inline_cpp_enabled = false
      end

      attr_reader :ast, :path, :load_path

      MACROS = %i[
        require
        require_relative
        load
        eval
        nat_ignore_require_relative
        nat_ignore_require
        include_str!
      ].freeze

      def expand
        ast.each_with_index do |node, i|
          next unless node.is_a?(Sexp)
          expanded =
            if (macro_name = macro?(node))
              run_macro(macro_name, node, path)
            elsif node.size > 1
              s(node[0], *expand_macros(node[1..-1], path))
            else
              node
            end
          next if expanded === node
          ast[i] = expanded
        end
        ast
      end

      def inline_cpp_enabled?
        @inline_cpp_enabled
      end

      private

      def expand_macros(ast, path)
        expander = MacroExpander.new(
          ast: ast,
          path: path,
          load_path: load_path,
          interpret: interpret?,
          compiler_context: @compiler_context,
        )
        result = expander.expand
        @inline_cpp_enabled ||= expander.inline_cpp_enabled?
        result
      end

      def macro?(node)
        return false unless node.is_a?(Sexp)
        if node[0..1] == s(:call, nil)
          _, _, name = node
          if MACROS.include?(name)
            name
          elsif @macros_enabled
            if name == :macro!
              name
            elsif @macros.key?(name)
              :user_macro
            end
          end
        elsif node.sexp_type == :iter
          macro?(node[1])
        end
      end

      def run_macro(macro_name, expr, current_path)
        send("macro_#{macro_name}", expr: expr, current_path: current_path)
      end

      def macro_user_macro(expr:, current_path:)
        _, _, name = expr
        macro = @macros[name]
        new_ast = VM.compile_and_run(macro, path: 'macro')
        expand_macros(new_ast, path: current_path)
      end

      def macro_macro!(expr:, current_path:)
        _, call, _, block = expr
        _, name = call.last
        @macros[name] = block
        s(:block)
      end

      def macro_require(expr:, current_path:) # rubocop:disable Lint/UnusedMethodArgument
        args = expr[3..]
        node = args.first
        raise ArgumentError, "Expected a String, but got #{node.inspect}" unless node.sexp_type == :str
        name = node[1]
        return s(:block) if name == 'tempfile' && interpret? # FIXME: not sure how to handle this actually
        if name == 'natalie/inline'
          @inline_cpp_enabled = true
          return s(:block)
        end
        ['.rb', '.cpp', ''].each do |extension|
          if (full_path = find_full_path(name + extension, base: Dir.pwd, search: true))
            return load_file(full_path, require_once: true)
          end
        end
        drop_load_error "cannot load such file #{name} at #{node.file}##{node.line}"
      end

      def macro_require_relative(expr:, current_path:)
        args = expr[3..]
        node = args.first
        raise ArgumentError, "Expected a String, but got #{node.inspect}" unless node.sexp_type == :str
        name = node[1]
        base = File.dirname(current_path)
        ['.rb', '.cpp', ''].each do |extension|
          if (full_path = find_full_path(name + extension, base: base, search: false))
            return load_file(full_path, require_once: true)
          end
        end
        drop_load_error "cannot load such file #{name} at #{node.file}##{node.line}"
      end

      def macro_load(expr:, current_path:) # rubocop:disable Lint/UnusedMethodArgument
        args = expr[3..]
        node = args.first
        raise ArgumentError, "Expected a String, but got #{node.inspect}" unless node.sexp_type == :str
        path = node.last
        full_path = find_full_path(path, base: Dir.pwd, search: true)
        return load_file(full_path, require_once: false) if full_path
        drop_load_error "cannot load such file -- #{path}"
      end

      def macro_eval(expr:, current_path:)
        args = expr[3..]
        node = args.first
        $stderr.puts 'FIXME: binding passed to eval() will be ignored.' if args.size > 1
        if node.sexp_type == :str
          begin
            Natalie::Parser.new(node[1], current_path).ast
          rescue SyntaxError => e
            # TODO: add a flag to raise syntax errors at compile time?
            s(:call, nil, :raise, s(:const, :SyntaxError), s(:str, e.message))
          end
        else
          s(:call, nil, :raise, s(:const, :SyntaxError), s(:str, 'eval() only works on static strings'))
        end
      end

      def macro_nat_ignore_require(expr:, current_path:) # rubocop:disable Lint/UnusedMethodArgument
        s(:false) # Script has not been loaded
      end

      def macro_nat_ignore_require_relative(expr:, current_path:) # rubocop:disable Lint/UnusedMethodArgument
        s(:false) # Script has not been loaded
      end

      def macro_include_str!(expr:, current_path:)
        args = expr[3..]
        node = args.first
        raise ArgumentError, "Expected a String, but got #{node.inspect}" unless node.sexp_type == :str
        name = node[1]
        if (full_path = find_full_path(name, base: File.dirname(current_path), search: false))
          s(:str, File.read(full_path))
        else
          raise IOError, "cannot find file #{name} at #{node.file}##{node.line}"
        end
      end

      def interpret?
        !!@interpret
      end

      def find_full_path(path, base:, search:)
        if path.start_with?(File::SEPARATOR)
          path if File.file?(path)
        elsif path.start_with?('.' + File::SEPARATOR)
          path = File.expand_path(path, base)
          path if File.file?(path)
        elsif search
          find_file_in_load_path(path)
        else
          path = File.expand_path(path, base)
          path if File.file?(path)
        end
      end

      def find_file_in_load_path(path)
        load_path.map { |d| File.join(d, path) }.detect { |p| File.file?(p) }
      end

      def load_file(path, require_once:)
        return load_cpp_file(path, require_once: require_once) if path =~ /\.cpp$/
        code = File.read(path)
        file_ast = Natalie::Parser.new(code, path).ast
        path_h = path.hash.to_s # the only point of this is to obscure the paths of the host system where natalie is run
        s(:block,
          s(:if,
            if require_once
              s(:call,
                s(:call,
                  s(:op_asgn_or, s(:gvar, :$NAT_LOADED_PATHS), s(:gasgn, :$NAT_LOADED_PATHS, s(:hash))),
                  :[],
                  s(:str, path_h),
                ),
                :!,
              )
            else
              s(:true)
            end,
            s(:block,
              expand_macros(file_ast, path),
              require_once ? s(:attrasgn, s(:gvar, :$NAT_LOADED_PATHS), :[]=, s(:str, path_h), s(:true)) : s(:true),
            ),
            s(:false),
          ),
        )
      end

      def load_cpp_file(path, require_once:)
        @inline_cpp_enabled = true # FIXME: this shouldn't be enabled everywhere; the user didn't ask for it
        name = File.split(path).last.split('.').first
        return if @compiler_context[:required_obj_files][path]
        @compiler_context[:required_obj_files][path] = name
        s(:block,
          s(:call, nil, :__inline__,
            s(:str, File.read(path))))
      end

      def drop_load_error(msg)
        STDERR.puts load_error_msg if @log_load_error
        s(:block, s(:call, nil, :raise, s(:call, s(:const, :LoadError), :new, s(:str, msg))))
      end

      def s(*items)
        sexp = Sexp.new
        items.each { |item| sexp << item }
        sexp
      end
    end
  end
end
