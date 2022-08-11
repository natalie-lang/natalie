require 'tempfile'
require_relative './compiler/pass1'
require_relative './compiler/pass2'
require_relative './compiler/pass3'
require_relative './compiler/pass4'
require_relative './compiler/instruction_manager'
require_relative './compiler/backends/cpp_backend'
require_relative './compiler/macro_expander'
require_relative '../../build/generated/numbers'

module Natalie
  class Compiler
    ROOT_DIR = File.expand_path('../../', __dir__)
    BUILD_DIR = File.join(ROOT_DIR, 'build')
    SRC_PATH = File.join(ROOT_DIR, 'src')
    INC_PATHS = [
      File.join(ROOT_DIR, 'include'),
      File.join(BUILD_DIR, 'onigmo/include'),
      File.join(BUILD_DIR, 'natalie_parser/include'),
    ]
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
      puts "cpp file path is: #{c_path}" if keep_cpp?
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
        repl: !!repl,
        vars: vars || {},
        inline_cpp_enabled: inline_cpp_enabled,
        compile_cxx_flags: cxx_flags,
        compile_ld_flags: [],
        source_path: @path,
      }
    end

    def out_path
      @out_path ||=
        begin
          out = Tempfile.create("natalie#{extension}")
          out.close
          out.path
        end
    end

    def extension
      RUBY_PLATFORM =~ /msys/ ? '.exe' : ''
    end

    def instructions
      @instructions ||= transform
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

    def transform
      ast = expand_macros(@ast, @path)

      @context = build_context

      instructions = Pass1.new(
        ast,
        inline_cpp_enabled: inline_cpp_enabled,
        compiler_context: @context,
      ).transform
      if debug == 'p1'
        Pass1.debug_instructions(instructions)
        exit
      end

      instructions = Pass2.new(
        instructions,
        compiler_context: @context,
      ).transform
      if debug == 'p2'
        Pass2.debug_instructions(instructions)
        exit
      end

      instructions = Pass3.new(instructions).transform
      if debug == 'p3'
        Pass3.debug_instructions(instructions)
        exit
      end

      instructions = Pass4.new(instructions).transform
      if debug == 'p4'
        Pass4.debug_instructions(instructions)
        exit
      end

      return instructions if options[:interpret]

      CppBackend.new(instructions, compiler_context: @context).generate
    end

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

    RELEASE_FLAGS = '-pthread -g -O2'
    DEBUG_FLAGS = '-pthread -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-value -Wno-unknown-warning-option -DNAT_GC_GUARD'
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
      expander = MacroExpander.new(
        ast: ast,
        path: path,
        load_path: load_path,
        interpret: interpret?,
        log_load_error: options[:log_load_error],
      )
      result = expander.expand
      @inline_cpp_enabled ||= expander.inline_cpp_enabled?
      result
    end

    # FIXME: implement pp
    if RUBY_ENGINE == 'natalie'
      def pp(obj)
        File.open('/tmp/pp.txt', 'w') { |f| f.write(obj.inspect) }
        puts `ruby -r pp -e "pp eval(File.read('/tmp/pp.txt'))"`
      end
    end
  end
end
