module Natalie
  class Compiler
    class MacroExpander
      include ComptimeValues

      class MacroError < StandardError; end
      class LoadPathMacroError < MacroError; end

      def initialize(ast:, path:, load_path:, interpret:, compiler_context:, log_load_error:, loaded_paths:, depth: 0)
        @ast = ast
        @path = path
        @load_path = load_path
        @interpret = interpret
        @compiler_context = compiler_context
        @inline_cpp_enabled = @compiler_context[:inline_cpp_enabled]
        @log_load_error = log_load_error
        @loaded_paths = loaded_paths
        @depth = depth
      end

      attr_reader :ast, :path, :load_path, :depth

      MACROS = %i[
        autoload
        eval
        include_str!
        load
        nat_ignore_require
        nat_ignore_require_relative
        require
        require_relative
      ].freeze

      def expand
        ast.each_with_index do |node, i|
          next unless node.is_a?(Sexp)
          expanded =
            if (macro_name = get_macro_name(node))
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

      private

      # FIXME: why do we instantitate this so much? Can we not stay within our own instance and just use method recursion?
      def expand_macros(ast, path)
        expander = MacroExpander.new(
          ast: ast,
          path: path,
          load_path: load_path,
          interpret: interpret?,
          compiler_context: @compiler_context,
          log_load_error: @log_load_error,
          loaded_paths: @loaded_paths,
          depth: @depth + 1,
        )
        expander.expand
      end

      def get_macro_name(node)
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
          get_macro_name(node[1])
        else
          get_hidden_macro_name(node)
        end
      end

      # "Hidden macros" are just regular-looking Ruby code we intercept at compile-time.
      # We will try to support common Ruby idioms here that cannot be done at runtime.
      def get_hidden_macro_name(node)
        if node[0..2] == s(:call, s(:gvar, :$LOAD_PATH), :<<)
          :append_load_path
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

      EXTENSIONS_TO_TRY = ['.rb', '.cpp', ''].freeze

      def macro_autoload(expr:, current_path:)
        args = expr[3..]
        const_node, path_node = args
        const = comptime_symbol(const_node)
        begin
          path = comptime_string(path_node)
        rescue ArgumentError
          return drop_load_error "cannot load such file #{path_node.inspect} at #{expr.file}##{expr.line}"
        end

        full_path = EXTENSIONS_TO_TRY.lazy.filter_map do |ext|
          find_full_path(path + ext, base: Dir.pwd, search: true)
        end.first

        unless full_path
          return drop_load_error "cannot load such file #{path} at #{expr.file}##{expr.line}"
        end

        body = load_file(full_path, require_once: true)
        expr.new(:autoload_const, const, full_path, body)
      end

      def macro_require(expr:, current_path:)
        args = expr[3..]
        name = comptime_string(args.first)
        return s(:block) if name == 'tempfile' && interpret? # FIXME: not sure how to handle this actually
        if name == 'natalie/inline'
          @inline_cpp_enabled[current_path] = true
          return s(:block)
        end
        EXTENSIONS_TO_TRY.each do |extension|
          if (full_path = find_full_path(name + extension, base: Dir.pwd, search: true))
            return load_file(full_path, require_once: true)
          end
        end
        drop_load_error "cannot load such file #{name} at #{expr.file}##{expr.line}"
      end

      def macro_require_relative(expr:, current_path:)
        args = expr[3..]
        name = comptime_string(args.first)
        base = File.dirname(current_path)
        EXTENSIONS_TO_TRY.each do |extension|
          if (full_path = find_full_path(name + extension, base: base, search: false))
            return load_file(full_path, require_once: true)
          end
        end
        drop_load_error "cannot load such file #{name} at #{expr.file}##{expr.line}"
      end

      def macro_load(expr:, current_path:) # rubocop:disable Lint/UnusedMethodArgument
        args = expr[3..]
        path = comptime_string(args.first)
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
        name = comptime_string(args.first)
        if (full_path = find_full_path(name, base: File.dirname(current_path), search: false))
          s(:str, File.read(full_path))
        else
          raise IOError, "cannot find file #{name} at #{node.file}##{node.line}"
        end
      end

      # $LOAD_PATH << some_expression
      def macro_append_load_path(expr:, current_path:)
        if @depth > 0
          return drop_error(:LoadError, "Cannot maniuplate $LOAD_PATH at runtime")
        end

        _, _, _, body = expr
        path_to_add = VM.compile_and_run(body.new(:block, body), path: current_path)

        unless path_to_add.is_a?(String) && File.directory?(path_to_add)
          raise LoadPathMacroError, "#{path_to_add.inspect} is not a directory"
        end

        load_path << path_to_add
        s(:nil)
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

        if @loaded_paths[path]
          if require_once
            s(:false)
          else
            s(:true)
          end
        else
          @loaded_paths[path] = true
          code = File.read(path)
          file_ast = Natalie::Parser.new(code, path).ast
          s(:block,
            s(:with_main,
              expand_macros(file_ast, path)),
            s(:true))
        end
      end

      def load_cpp_file(path, require_once:)
        name = File.split(path).last.split('.').first
        return s(:false) if @compiler_context[:required_cpp_files][path]
        @compiler_context[:required_cpp_files][path] = name
        cpp_source = File.read(path)
        init_function = "Value init(Env *env, Value self)"
        transformed_init_function = "Value init_#{name}(Env *env, Value self)"
        if cpp_source.include?(init_function);
          cpp_source.sub!(init_function, transformed_init_function)
        else
          $stderr.puts "Expected #{path} to contain function: `#{init_function}`\n" \
                       "...which will be rewritten to: `#{transformed_init_function}`"
          raise CompileError, "could not load #{name}"
        end
        s(:block,
          s(:require_cpp_file, nil, :__inline__, s(:str, cpp_source)),
          s(:true))
      end

      def drop_error(exception_class, message)
        warn("#{exception_class}: #{message}")
        s(:call,
          nil,
          :raise,
          s(:const, exception_class),
          s(:str, message))
      end

      def drop_load_error(msg)
        # TODO: delegate to drop_error above
        STDERR.puts(msg) if @log_load_error
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
