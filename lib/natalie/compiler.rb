require 'tempfile'
require 'sexp_processor'
require_relative './compiler/pass1'
require_relative './compiler/pass2'
require_relative './compiler/pass3'
require_relative './compiler/pass4'

module Natalie
  class Compiler
    ROOT_DIR = File.expand_path('../../', __dir__)
    BUILD_DIR = File.join(ROOT_DIR, 'build')
    SRC_PATH = File.join(ROOT_DIR, 'src')
    INC_PATHS = [
      File.join(ROOT_DIR, 'include'),
      File.join(BUILD_DIR, 'include'),
      File.join(BUILD_DIR, 'include/gdtoa'),
      File.join(BUILD_DIR, '_deps/pegparser-src/include'),
    ]
    LIB_PATHS = [
      BUILD_DIR,
      File.join(BUILD_DIR, '_deps/pegparser-build'),
    ]
    LIBRARIES = %W[
      -lnatalie
      -lgc
      -lgccpp
      -lgdtoa
      -lhashmap
      -lm
      -lonigmo
      -lPEGParser
    ]

    RB_LIB_PATH = File.expand_path('..', __dir__)

    MAIN_TEMPLATE = File.read(File.join(SRC_PATH, 'main.cpp'))
    OBJ_TEMPLATE = <<-EOF
      #{MAIN_TEMPLATE.split(/\/\* end of front matter \*\//).first}

      /*TOP*/

      Value *obj_%{name}(Env *env, Value *self) {
        /*BODY*/
        return env->nil_obj();
      }
    EOF

    class CompileError < StandardError; end

    def initialize(ast, path, options = {})
      @ast = ast
      @var_num = 0
      @path = path
      @options = options
      @required = {}
    end

    attr_accessor :ast, :write_obj, :repl, :out_path, :context, :vars, :options, :c_path, :inline_cpp_enabled

    attr_writer :load_path

    def compile
      return write_file if write_obj
      check_build
      write_file
      compile_c_to_binary
    end

    def compile_c_to_binary
      cmd = compiler_command
      out = `#{cmd} 2>&1`
      File.unlink(@c_path) unless debug || build == 'coverage' || $? != 0
      $stderr.puts out if out.strip != ''
      raise CompileError.new('There was an error compiling.') if $? != 0
    end

    def check_build
      unless File.exist?(File.join(BUILD_DIR, 'libnatalie.a'))
        puts 'please run: make build'
        exit 1
      end
    end

    def write_file
      c = to_c
      if write_obj
        File.write(write_obj, c)
      else
        temp_c = Tempfile.create('natalie.cpp')
        temp_c.write(c)
        temp_c.close
        @c_path = temp_c.path
      end
    end

    def build_context
      {
        var_prefix: var_prefix,
        var_num: 0,
        template: template,
        repl: repl,
        vars: vars || {},
        inline_cpp_enabled: inline_cpp_enabled,
      }
    end

    def transform(ast)
      @context = build_context

      ast = Pass1.new(@context).go(ast)
      if debug == 'p1'
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

      Pass4.new(@context).go(ast)
    end

    def to_c
      @ast = expand_macros(@ast, @path)
      reindent(transform(@ast))
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

    def inc_paths
      INC_PATHS.map { |path| "-I #{path}" }.join(' ')
    end

    def compiler_command
      if clang?
        [
          cc,
          build_flags,
          ENV['NAT_CXX_FLAGS'],
          (shared? ? '-fPIC -shared' : ''),
          inc_paths,
          "-o #{out_path}",
          "#{BUILD_DIR}/libnatalie.a",
          LIB_PATHS.map { |path| "-L #{path}" }.join(' '),
          libraries.join(' '),
          "-x c++ -std=c++17",
          (@c_path || 'code.cpp'),
        ].map(&:to_s).join(' ')
      else
        [
          cc,
          build_flags,
          ENV['NAT_CXX_FLAGS'],
          (shared? ? '-fPIC -shared' : ''),
          inc_paths,
          "-o #{out_path}",
          "-x c++ -std=c++17",
          (@c_path || 'code.cpp'),
          LIB_PATHS.map { |path| "-L #{path}" }.join(' '),
          libraries.join(' '),
        ].map(&:to_s).join(' ')
      end
    end

    private

    def clang?
      return @clang if defined?(@clang)
      @clang = !!(`#{cc} --version` =~ /clang/)
    end

    def libraries
      if ENV['NAT_CXX_FLAGS'] =~ /NAT_GC_DISABLE/
        LIBRARIES - %w[-lgc -lgccpp]
      else
        LIBRARIES
      end
    end

    def cc
      ENV['CXX'] || 'c++'
    end

    def shared?
      !!repl
    end

    RELEASE_FLAGS = '-O1'
    DEBUG_FLAGS = '-g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unknown-warning-option'
    COVERAGE_FLAGS = '-fprofile-arcs -ftest-coverage'

    def build_flags
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
      if write_obj
        "#{obj_name}_"
      else
        ''
      end
    end

    def obj_name
      write_obj.sub(/\.cpp/, '').split('/').last
    end

    def template
      if write_obj
        OBJ_TEMPLATE % { name: obj_name }
      else
        MAIN_TEMPLATE
      end
    end

    def expand_macros(ast, path)
      ast.each_with_index do |node, i|
        if macro?(node)
          expanded = run_macro(node, path)
          raise 'bad node' if expanded.sexp_type != :block
          ast[i] = expanded
        end
      end
      ast
    end

    def macro?(node)
      return false unless node[0..1] == s(:call, nil)
      %i[require require_relative load].include?(node[2])
    end

    def run_macro(expr, path)
      (_, _, macro, *args) = expr
      send("macro_#{macro}", *args, path)
    end

    REQUIRE_EXTENSIONS = %w[nat rb]

    def macro_require(node, current_path)
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
      raise LoadError, "cannot load such file #{name} at #{node.file}##{node.line}"
    end

    def macro_require_relative(node, current_path)
      name = node[1]
      if (full_path = find_full_path("#{name}.rb", base: File.dirname(current_path), search: false))
        return load_file(full_path, require_once: true)
      elsif (full_path = find_full_path(name, base: File.dirname(current_path), search: false))
        return load_file(full_path, require_once: true)
      end
      raise LoadError, "cannot load such file #{name} at #{node.file}##{node.line}"
    end

    def macro_load(node, _)
      path = node.last
      full_path = find_full_path(path, base: Dir.pwd, search: true)
      if full_path
        load_file(full_path, require_once: false)
      else
        raise LoadError, "cannot load such file -- #{path}"
      end
    end

    def load_file(path, require_once:)
      return s(:block) if require_once && @required[path]
      @required[path] = true
      code = File.read(path)
      file_ast = Natalie::Parser.new(code, path).ast
      expand_macros(file_ast, path)
    end

    def find_full_path(path, base:, search:)
      if path.start_with?(File::SEPARATOR)
        path if File.exist?(path)
      elsif path.start_with?('.' + File::SEPARATOR)
        path = File.expand_path(path, base)
        path if File.exist?(path)
      elsif search
        find_file_in_load_path(path)
      else
        path = File.expand_path(path, base)
        path if File.exist?(path)
      end
    end

    def find_file_in_load_path(path)
      load_path.map { |d| File.join(d, path) }.detect { |p| File.exist?(p) }
    end

    def reindent(code)
      out = []
      indent = 0
      code.split("\n").each do |line|
        indent -= 4 if line =~ /^\s*\}/
        indent = [0, indent].max
        out << line.sub(/^\s*/, ' ' * indent)
        indent += 4 if line.end_with?('{')
      end
      out.join("\n")
    end
  end
end
