require 'tempfile'
require_relative '../../build/generated/numbers'
require_relative './compiler/backends/cpp_backend'
require_relative './compiler/comptime_values'
require_relative './compiler/instruction_manager'
require_relative './compiler/loaded_file'
require_relative './compiler/macro_expander'
require_relative './compiler/pass1'
require_relative './compiler/pass2'
require_relative './compiler/pass3'
require_relative './compiler/pass4'
require_relative './parser'

module Natalie
  class Compiler
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
                  :context,
                  :c_path,
                  :inline_cpp_enabled,
                  :options,
                  :repl,
                  :repl_num,
                  :vars,
                  :write_obj_path

    attr_writer :load_path, :out_path

    def compile
      return backend.compile_to_object if write_obj_path

      backend.compile_to_binary
    end

    def write_file_for_debugging
      backend.write_file_for_debugging
    end

    def compiler_command
      backend.compiler_command
    end

    def build_context
      {
        inline_cpp_enabled:  inline_cpp_enabled,
        repl:                repl?,
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

    def repl?
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

      main_file = LoadedFile.new(path: @path, instructions: instructions)
      files = [main_file] + @context[:required_ruby_files].values
      files.each do |file_info|
        {
          'p2' => Pass2,
          'p3' => Pass3,
          'p4' => Pass4,
        }.each do |short_name, klass|
          file_info.instructions = klass.new(
            file_info.instructions,
            compiler_context: @context,
          ).transform
          if debug == short_name
            klass.debug_instructions(instructions)
            exit
          end
        end
      end

      main_file.instructions
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
