require 'tempfile'
require 'sexp_processor'
require_relative './compiler/pass1'
require_relative './compiler/pass2'
require_relative './compiler/pass3'
require_relative './compiler/pass4'
require_relative './compiler/pass5'

module Natalie
  class Compiler
    SRC_PATH = File.expand_path('../../src', __dir__)
    OBJ_PATH = File.expand_path('../../obj', __dir__)
    ONIGMO_SRC_PATH = File.expand_path('../../ext/onigmo', __dir__)
    ONIGMO_LIB_PATH = File.expand_path('../../ext/onigmo/.libs', __dir__)

    MAIN_TEMPLATE = File.read(File.join(SRC_PATH, 'main.c'))
    OBJ_TEMPLATE = <<-EOF
      #{MAIN_TEMPLATE.split(/\/\* end of front matter \*\//).first}

      /*TOP*/

      NatObject *obj_%{name}(NatEnv *env, NatObject *self) {
        /*BODY*/
        return NAT_NIL;
      }
    EOF

    class CompileError < StandardError; end

    def initialize(ast, path, options = {})
      @ast = ast
      @var_num = 0
      @path = path
      @options = options
    end

    attr_accessor :ast, :compile_to_object_file, :repl, :out_path, :context, :vars, :options

    attr_writer :load_path

    def compile
      check_build
      write_file
      cmd = compiler_command
      puts cmd if debug == 'compiler-command'
      out = `#{cmd}`
      File.unlink(@c_path) unless debug || build == 'coverage'
      $stderr.puts out if debug || $? != 0
      raise 'There was an error compiling.' if $? != 0
    end

    def c_files_to_compile
      Dir[File.join(SRC_PATH, '*.c')].grep_v(/main\.c$/)
    end

    def check_build
      if Dir[File.join(OBJ_PATH, '*.o')].none?
        puts 'please run: make build'
        exit 1
      end
    end

    def write_file
      c = to_c
      puts c if debug
      temp_c = Tempfile.create('natalie.c')
      temp_c.write(c)
      temp_c.close
      @c_path = temp_c.path
    end

    def build_context
      {
        var_prefix: var_prefix,
        var_num: 0,
        template: template,
        repl: repl,
        vars: vars || {}
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

      ast = Pass4.new(@context).go(ast)

      Pass5.new(@context, ast).go
    end

    def to_c
      @ast = expand_macros(@ast, @path)
      reindent(transform(@ast))
    end

    def load_path
      Array(@load_path)
    end

    def ld_library_path
      self.class.ld_library_path
    end

    def self.ld_library_path
      ONIGMO_LIB_PATH
    end

    def debug
      options[:debug]
    end

    def build
      options[:build]
    end

    private

    def compiler_command
      if compile_to_object_file
        "#{cc} #{build_flags} -I #{SRC_PATH} -I #{ONIGMO_SRC_PATH} -fPIC -x c -c #{@c_path} -o #{out_path} 2>&1"
      else
        "#{cc} #{build_flags} #{shared? ? '-fPIC -shared' : ''} -I #{SRC_PATH} -I #{ONIGMO_SRC_PATH} -o #{out_path} -L #{ld_library_path} #{OBJ_PATH}/*.o #{OBJ_PATH}/nat/*.o #{ONIGMO_LIB_PATH}/libonigmo.a -x c #{@c_path} 2>&1"
      end
    end

    def cc
      ENV['CC'] || 'cc'
    end

    def shared?
      !!repl
    end

    RELEASE_FLAGS = '-O3 -pthread'
    DEBUG_FLAGS = '-g -Wall -Wextra -Wno-unused-parameter -pthread'
    COVERAGE_FLAGS = '-fprofile-arcs -ftest-coverage'
    NOGC_FLAGS = '-DNAT_DISABLE_GC'

    def build_flags
      case build
      when 'release'
        RELEASE_FLAGS
      when 'debug', nil
        DEBUG_FLAGS
      when 'coverage'
        DEBUG_FLAGS + ' ' + COVERAGE_FLAGS
      when 'nogc'
        DEBUG_FLAGS + ' ' + NOGC_FLAGS
      else
        raise "unknown build mode: #{build.inspect}"
      end
    end

    def var_prefix
      if compile_to_object_file
        "#{obj_name}_"
      else
        ''
      end
    end

    def obj_name
      out_path.sub(/.*obj\//, '').sub(/\.o$/, '').tr('/', '_')
    end

    def template
      if compile_to_object_file
        OBJ_TEMPLATE % { name: obj_name }
      elsif repl
        MAIN_TEMPLATE.sub(/env->global_env->gc_enabled = true;/, '')
      else
        MAIN_TEMPLATE
      end
    end

    def expand_macros(ast, path)
      (0...(ast.size)).reverse_each do |i|
        node = ast[i]
        if macro?(node)
          ast[i,1] = run_macro(node, path)
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
      REQUIRE_EXTENSIONS.each do |extension|
        path = "#{name}.#{extension}"
        next unless full_path = find_full_path(path, base: Dir.pwd, search: true)
        return load_file(full_path)
      end
      raise LoadError, "cannot load such file -- #{name}.{#{REQUIRE_EXTENSIONS.join(',')}}"
    end

    def macro_require_relative(node, current_path)
      name = node[1]
      REQUIRE_EXTENSIONS.each do |extension|
        path = "#{name}.#{extension}"
        next unless full_path = find_full_path(path, base: File.dirname(current_path), search: false)
        return load_file(full_path)
      end
      raise LoadError, "cannot load such file -- #{name}.{#{REQUIRE_EXTENSIONS.join(',')}}"
    end

    def macro_load(node, _)
      path = node.last
      full_path = find_full_path(path, base: Dir.pwd, search: true)
      if full_path
        load_file(full_path)
      else
        raise LoadError, "cannot load such file -- #{path}"
      end
    end

    def load_file(path)
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
        out << line.sub(/^\s*/, ' ' * indent)
        indent += 4 if line.end_with?('{')
      end
      out.join("\n")
    end
  end
end
