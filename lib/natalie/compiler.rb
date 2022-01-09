require 'tempfile'
require_relative '../sexp_processor'
require_relative './compiler/pass0c'
require_relative './compiler/pass1'
require_relative './compiler/pass1b'
require_relative './compiler/pass1r'
require_relative './compiler/pass2'
require_relative './compiler/pass3'
require_relative './compiler/pass4'
require_relative './compiler/pass4i'

module Natalie
  class Compiler
    ROOT_DIR = File.expand_path('../../', __dir__)
    BUILD_DIR = File.join(ROOT_DIR, 'build')
    SRC_PATH = File.join(ROOT_DIR, 'src')
    INC_PATHS = [File.join(ROOT_DIR, 'include'), File.join(BUILD_DIR, 'onigmo/include')]
    LIB_PATHS = [BUILD_DIR]
    LIBRARIES = %W[-lnatalie]

    RB_LIB_PATH = File.expand_path('..', __dir__)

    MAIN_TEMPLATE = File.read(File.join(SRC_PATH, 'main.cpp'))
    OBJ_TEMPLATE = File.read(File.join(SRC_PATH, 'obj_unit.cpp'))

    class CompileError < StandardError
    end

    def initialize(ast, path, options = {})
      @ast = ast
      @var_num = 0
      @path = path
      @options = options
      @cxx_flags = []
    end

    attr_accessor :ast,
                  :write_obj_path,
                  :repl,
                  :repl_num,
                  :out_path,
                  :context,
                  :vars,
                  :options,
                  :c_path,
                  :inline_cpp_enabled,
                  :cxx_flags

    attr_writer :load_path

    # FIXME: this is only used for cpp right now
    def compile
      return write_file if write_obj_path
      check_build
      write_file
      compile_c_to_binary
    end

    def compile_c_to_binary
      cmd = compiler_command
      out = `#{cmd} 2>&1`
      File.unlink(@c_path) unless keep_cpp? || $? != 0
      p "cpp file path is: #{c_path}" if keep_cpp?
      $stderr.puts out if out.strip != ''
      raise CompileError.new('There was an error compiling.') if $? != 0
    end

    def check_build
      unless File.file?(File.join(BUILD_DIR, 'libnatalie.a'))
        puts 'please run: rake'
        exit 1
      end
    end

    def write_file
      cpp = instructions
      if write_obj_path
        File.write(write_obj_path, cpp)
      else
        temp_c = Tempfile.create('natalie.cpp')
        temp_c.write(cpp)
        temp_c.close
        @c_path = temp_c.path
      end
    end

    def build_context
      {
        var_prefix: var_prefix,
        var_num: 0,
        template: template,
        is_obj: !!write_obj_path,
        repl: repl,
        vars: vars || {},
        inline_cpp_enabled: inline_cpp_enabled,
        compile_cxx_flags: cxx_flags,
        compile_ld_flags: [],
        source_path: @path,
      }
    end

    def transform(ast)
      @context = build_context

      ast = Pass0c.new(@context).go(ast)
      if debug == 'p0c'
        pp ast
        exit
      end

      ast = Pass1.new(@context).go(ast)
      if debug == 'p1'
        pp ast
        exit
      end

      ast = Pass1b.new(@context).go(ast)
      if debug == 'p1b'
        pp ast
        exit
      end

      ast = Pass1r.new(@context).go(ast)
      if debug == 'p1r'
        pp ast
        exit
      end

      ast = Pass2.new(@context).go(ast)
      if debug == 'p2'
        pp ast
        exit
      end

      ast = Pass3.new(@context).go(ast)
      if debug == 'p3'
        pp ast
        exit
      end

      interpret? ? Pass4i.new(@context).go(ast) : Pass4.new(@context).go(ast)
    end

    def instructions
      @ast = expand_macros(@ast, @path)
      transform(@ast)
    end

    def load_path
      Array(@load_path) + [RB_LIB_PATH]
    end

    def debug
      options[:debug]
    end

    def build
      options[:build]
    end

    def keep_cpp?
      !!(debug || options[:keep_cpp])
    end

    def log_load_error
      options[:log_load_error]
    end

    def interpret?
      !!options[:interpret]
    end

    def inc_paths
      INC_PATHS.map { |path| "-I #{path}" }.join(' ')
    end

    def compiler_command
      [
        cc,
        build_flags,
        (shared? ? '-fPIC -shared' : ''),
        inc_paths,
        "-o #{out_path}",
        '-x c++ -std=c++17',
        (@c_path || 'code.cpp'),
        LIB_PATHS.map { |path| "-L #{path}" }.join(' '),
        libraries.join(' '),
        link_flags,
      ].map(&:to_s).join(' ')
    end

    private

    def clang?
      return @clang if defined?(@clang)
      @clang = !!(`#{cc} --version` =~ /clang/)
    end

    def libraries
      LIBRARIES
    end

    def cc
      ENV['CXX'] || 'c++'
    end

    def shared?
      !!repl
    end

    RELEASE_FLAGS = '-pthread -O1'
    DEBUG_FLAGS =
      '-pthread -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unknown-warning-option'
    COVERAGE_FLAGS = '-fprofile-arcs -ftest-coverage'

    def build_flags
      "#{base_build_flags} #{ENV['NAT_CXX_FLAGS']} #{@context[:compile_cxx_flags].join(' ')}"
    end

    def link_flags
      (@context[:compile_ld_flags] - unnecessary_link_flags).join(' ')
    end

    def unnecessary_link_flags
      RUBY_PLATFORM =~ /openbsd/ ? ['-ldl'] : []
    end

    def base_build_flags
      case build
      when 'release'
        RELEASE_FLAGS
      when 'debug', nil
        DEBUG_FLAGS
      when 'coverage'
        DEBUG_FLAGS + ' ' + COVERAGE_FLAGS
      else
        raise "unknown build mode: #{build.inspect}"
      end
    end

    def var_prefix
      if write_obj_path
        "#{obj_name}_"
      elsif repl
        "repl#{repl_num}_"
      else
        ''
      end
    end

    def obj_name
      write_obj_path.sub(/\.rb\.cpp/, '').split('/').last
    end

    def template
      write_obj_path ? OBJ_TEMPLATE.sub(/init_obj/, "init_obj_#{obj_name}") : MAIN_TEMPLATE
    end

    def expand_macros(ast, path)
      ast.each_with_index do |node, i|
        next unless node.is_a?(Sexp)
        expanded =
          if macro?(node)
            run_macro(node, path)
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

    def macro?(node)
      return false unless node.is_a?(Sexp)
      return false unless node[0..1] == s(:call, nil)
      %i[require require_relative load eval nat_ignore_require_relative nat_ignore_require].include?(node[2])
    end

    def run_macro(expr, current_path)
      _, _, macro, *args = expr
      send("macro_#{macro}", args: args, current_path: current_path)
    end

    def macro_require(args:, current_path:)
      node = args.first
      raise ArgumentError, "Expected a String, but got #{node.inspect}" unless node.sexp_type == :str
      name = node[1]
      if name == 'natalie/inline'
        @inline_cpp_enabled = true
        return s(:block)
      end
      if (full_path = find_full_path("#{name}.rb", base: Dir.pwd, search: true))
        return load_file(full_path, require_once: true)
      elsif (full_path = find_full_path(name, base: Dir.pwd, search: true))
        return load_file(full_path, require_once: true)
      end
      drop_load_error "cannot load such file #{name} at #{node.file}##{node.line}"
    end

    def macro_require_relative(args:, current_path:)
      node = args.first
      raise ArgumentError, "Expected a String, but got #{node.inspect}" unless node.sexp_type == :str
      name = node[1]
      if (full_path = find_full_path("#{name}.rb", base: File.dirname(current_path), search: false))
        return load_file(full_path, require_once: true)
      elsif (full_path = find_full_path(name, base: File.dirname(current_path), search: false))
        return load_file(full_path, require_once: true)
      end
      drop_load_error "cannot load such file #{name} at #{node.file}##{node.line}"
    end

    def macro_load(args:, current_path:)
      node = args.first
      raise ArgumentError, "Expected a String, but got #{node.inspect}" unless node.sexp_type == :str
      path = node.last
      full_path = find_full_path(path, base: Dir.pwd, search: true)
      return load_file(full_path, require_once: false) if full_path
      drop_load_error "cannot load such file -- #{path}"
    end

    def macro_eval(args:, current_path:)
      node = args.first
      $stderr.puts 'FIXME: binding passed to eval() will be ignored.' if args.size > 1
      if node.sexp_type == :str
        begin
          Natalie::Parser.new(node[1], current_path).ast
        rescue Racc::ParseError => e
          # TODO: add a flag to raise syntax errors at compile time?
          s(:call, nil, :raise, s(:const, :SyntaxError), s(:str, e.message))
        end
      else
        s(:call, nil, :raise, s(:const, :SyntaxError), s(:str, 'eval() only works on static strings'))
      end
    end

    def macro_nat_ignore_require(args:, current_path:)
      s(:false) # Script has not been loaded
    end

    def macro_nat_ignore_require_relative(args:, current_path:)
      s(:false) # Script has not been loaded
    end

    def drop_load_error(msg)
      STDERR.puts load_error_msg if log_load_error
      s(:block, s(:call, nil, :raise, s(:call, s(:const, :LoadError), :new, s(:str, msg))))
    end

    def load_file(path, require_once:)
      code = File.read(path)
      file_ast = Natalie::Parser.new(code, path).ast
      path_h = path.hash.to_s # the only point of this is to obscure the paths of the host system where natalie is run
      s(
        :block,
        s(
          :if,
          if require_once
            s(
              :call,
              s(
                :call,
                s(:op_asgn_or, s(:gvar, :$NAT_LOADED_PATHS), s(:gasgn, :$NAT_LOADED_PATHS, s(:hash))),
                :[],
                s(:str, path_h),
              ),
              :!,
            )
          else
            s(:true)
          end,
          s(
            :block,
            expand_macros(file_ast, path),
            require_once ? s(:attrasgn, s(:gvar, :$NAT_LOADED_PATHS), :[]=, s(:str, path_h), s(:true)) : s(:true),
          ),
          s(:false),
        ),
      )
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

    # FIXME: implement pp
    if RUBY_ENGINE == 'natalie'
      def pp(obj)
        File.open('/tmp/pp.txt', 'w') { |f| f.write(obj.inspect) }
        puts `ruby -r ruby_parser -r pp -e "pp eval(File.read('/tmp/pp.txt'))"`
      end
    end
  end
end
