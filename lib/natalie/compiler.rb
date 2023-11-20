require 'tempfile'
require_relative './parser'
require_relative './compiler/comptime_values'
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
    ROOT_DIR = File.expand_path('../../', __dir__)
    BUILD_DIR = File.join(ROOT_DIR, 'build')

    DL_EXT = RbConfig::CONFIG['DLEXT']

    RB_LIB_PATH = File.expand_path('..', __dir__)

    class CompileError < StandardError
    end

    def initialize(ast, path, options = {})
      @ast = ast
      @var_num = 0
      @path = path
      @options = options
      @inline_cpp_enabled = {}
    end

    attr_accessor :ast,
                  :write_obj_path,
                  :repl,
                  :repl_num,
                  :context,
                  :vars,
                  :options,
                  :c_path,
                  :inline_cpp_enabled

    attr_writer :load_path, :out_path

    def compile
      return backend.compile_to_object if write_obj_path

      backend.compile_to_binary
    end

    def build_context
      {
        inline_cpp_enabled:  inline_cpp_enabled,
        repl:                !!repl,
        required_cpp_files:  {},
        required_ruby_files: {},
        source_path:         @path,
        var_num:             0,
        vars:                vars || {},
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

    def backend
      @backend ||= CppBackend.new(instructions, compiler: self, compiler_context: @context)
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

    def dynamic_linking?
      !!options[:dynamic_linking]
    end

    def shared?
      !!repl
    end

    private

    def transform
      @context = build_context

      keep_final_value_on_stack = options[:interpret]
      instructions = Pass1.new(
        ast,
        compiler_context: @context,
        macro_expander:   macro_expander
      ).transform(
        used: keep_final_value_on_stack
      )
      if debug == 'p1'
        Pass1.debug_instructions(instructions)
        exit
      end

      main_file = { instructions: instructions }
      files = [main_file] + @context[:required_ruby_files].values
      files.each do |file_info|
        {
          'p2' => Pass2,
          'p3' => Pass3,
          'p4' => Pass4,
        }.each do |short_name, klass|
          file_info[:instructions] = klass.new(
            file_info.fetch(:instructions),
            compiler_context: @context,
          ).transform
          if debug == short_name
            klass.debug_instructions(instructions)
            exit
          end
        end
      end

      main_file.fetch(:instructions)
    end

    def macro_expander
      @macro_expander ||= MacroExpander.new(
        path:             @path,
        load_path:        load_path,
        interpret:        interpret?,
        log_load_error:   options[:log_load_error],
        compiler_context: @context,
      )
    end
  end
end
