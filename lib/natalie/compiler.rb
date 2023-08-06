require 'tempfile'
require_relative './compiler/flags'
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
    include Flags

    ROOT_DIR = File.expand_path('../../', __dir__)
    BUILD_DIR = File.join(ROOT_DIR, 'build')
    SRC_PATH = File.join(ROOT_DIR, 'src')
    INC_PATHS = [
      File.join(ROOT_DIR, 'include'),
      File.join(ROOT_DIR, 'ext/tm/include'),
      File.join(BUILD_DIR),
      File.join(BUILD_DIR, 'onigmo/include'),
      File.join(BUILD_DIR, 'natalie_parser/include'),
    ]
    LIB_PATHS = [
      BUILD_DIR,
      File.join(BUILD_DIR, 'onigmo/lib'),
      File.join(BUILD_DIR, 'zlib'),
    ]
    if system('pkg-config --exists openssl')
      unless (openssl_inc_path = `pkg-config --cflags openssl`.strip).empty?
        INC_PATHS << openssl_inc_path.sub(/^\-I/, '')
      end
      unless (openssl_lib_path = `pkg-config --libs-only-L openssl`.strip).empty?
        LIB_PATHS << openssl_lib_path.sub(/^\-L/, '')
      end
    end
    SO_EXT = RUBY_PLATFORM =~ /darwin/ ? 'bundle' : 'so'

    # When running `bin/natalie script.rb`, we use dynamic linking to speed things up.
    LIBRARIES_FOR_DYNAMIC_LINKING = %w[
      -lnatalie_base
      -lnatalie_parser
      -lonigmo
    ]

    # When using the REPL or compiling a binary with the `-c` option,
    # we use static linking for compatibility.
    LIBRARIES_FOR_STATIC_LINKING = %w[
      -lnatalie
      -lcrypt
    ]

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
      @inline_cpp_enabled = {}
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
      unless File.file?(File.join(BUILD_DIR, "libnatalie_base.#{SO_EXT}"))
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
        required_cpp_files: {},
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
      start_profiling if options[:profile] == :compiler
      @context = build_context

      ast = expand_macros

      keep_final_value_on_stack = options[:interpret]
      instructions = Pass1.new(ast, compiler_context: @context)
                          .transform(used: keep_final_value_on_stack)
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
    ensure
      stop_profiling if options[:profile] == :compiler
    end

    def libraries
      if options[:dynamic_linking]
        LIBRARIES_FOR_DYNAMIC_LINKING
      else
        LIBRARIES_FOR_STATIC_LINKING
      end
    end

    def cc
      ENV['CXX'] || 'c++'
    end

    def shared?
      !!repl
    end

    def build_flags
      "#{base_build_flags.join(' ')} #{ENV['NAT_CXX_FLAGS']} #{@context[:compile_cxx_flags].join(' ')}"
    end

    def link_flags
      (@context[:compile_ld_flags].uniq - unnecessary_link_flags).join(' ')
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
        COVERAGE_FLAGS
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
      # FIXME: I don't like that this method "knows" how to ignore the build/generated directory
      # Maybe we need another arg to specify the init name...
      write_obj_path.sub(/\.rb\.cpp/, '').sub(%r{.*build/generated/}, '').tr('/', '_')
    end

    def template
      write_obj_path ? OBJ_TEMPLATE.gsub(/OBJ_NAME/, obj_name) : MAIN_TEMPLATE
    end

    def expand_macros
      expander = MacroExpander.new(
        ast: @ast,
        path: @path,
        load_path: load_path,
        interpret: interpret?,
        log_load_error: options[:log_load_error],
        compiler_context: @context,
      )
      expander.expand
    end

    def start_profiling
      require 'stackprof'
      StackProf.start(mode: :wall, raw: true)
    end

    PROFILES_PATH = '/tmp/natalie/profiles'

    def stop_profiling
      StackProf.stop
      # You can install speedscope using npm install -g speedscope
      if use_speedscope?
        require 'json'
        FileUtils.mkdir_p(PROFILES_PATH)
        profile = "#{PROFILES_PATH}/#{Time.new.to_i}.dump"
        File.write(profile, JSON.generate(StackProf.results))
        system("speedscope #{profile}")
      else
        StackProf::Report.new(StackProf.results).print_text
      end
    end

    def use_speedscope?
      system("speedscope --version > /dev/null 2>&1")
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
