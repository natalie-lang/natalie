require 'tempfile'
require_relative '../../build/generated/numbers'
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
    DEFAULT_BACKEND = 'llvm'.freeze

    class CompileError < StandardError
    end

    def initialize(ast:, path:, encoding: Encoding::UTF_8, warnings: [], data_loc: nil, options: {})
      @ast = ast
      @path = path
      @encoding = encoding
      @warnings = warnings
      @data_loc = data_loc
      @options = options
      @inline_cpp_enabled = {}
    end

    attr_accessor :ast, :warnings, :context, :inline_cpp_enabled, :options, :repl, :repl_num, :vars

    attr_writer :load_path

    def out_path=(path)
      @out_path = path
      @temporary_output = false
    end

    def temporary_output? = !!(options[:temporary_output] || @temporary_output)

    def compile
      if compilation_type == 'object'
        backend.compile_to_object_file(@out_path)
      else
        backend.compile_to_binary
      end
    end

    def write_files_for_debugging
      backend.write_files_for_debugging
    end

    def write_assembly_files
      backend.write_assembly_files
    end

    def build_context
      {
        inline_cpp_enabled: inline_cpp_enabled,
        repl: repl?,
        required_cpp_files: {
        },
        required_ruby_files: {
        },
        source_path: @path,
        vars: vars || {},
      }
    end

    def out_path
      @out_path ||=
        begin
          @temporary_output = true
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
      @backend ||=
        begin
          klass =
            case options.fetch(:backend, DEFAULT_BACKEND)
            when 'cpp'
              Kernel.require_relative './compiler/backends/cpp_backend' unless RUBY_ENGINE == 'natalie'
              CppBackend
            when 'llvm'
              # An explicit receiver keeps Natalie's require macro from bundling
              # MRI's Fiddle dependency into the self-hosted C++ compiler.
              Kernel.require_relative './compiler/backends/llvm_backend' unless RUBY_ENGINE == 'natalie'
              LlvmBackend
            else
              raise CompileError, "Unknown backend: #{options[:backend]}"
            end
          klass.new(instructions, compiler: self, compiler_context: @context)
        end
    end

    def load_path
      Array(@load_path) + [RB_LIB_PATH]
    end

    def debug = options[:debug]
    def build = options[:build]
    def build_dir = options[:build_dir]
    def build_quietly? = !!options[:build_quietly]
    def compilation_type = options.fetch(:compilation_type, 'executable')
    def dynamic_linking? = !!options[:dynamic_linking]
    def repl? = !!repl
    def frozen_string_literal? = !!options[:frozen_string_literal]
    def profile_app! = backend.profile_app!
    def print_objects! = backend.print_objects!

    def print_source_path?
      options[:keep_source]
    end

    def keep_source?
      !!((debug && debug != 'cc-cmd') || options[:keep_source]) || build_dir || compilation_type == 'object'
    end

    private

    def data_loc
      return nil if @data_loc.nil?

      { path: @path, data_loc: @data_loc }
    end

    def transform
      @context = build_context

      main_file = LoadedFile.new(path: @path, encoding: @encoding)

      instructions =
        Pass1.new(
          ast,
          compiler_context: @context,
          data_loc: data_loc,
          macro_expander: macro_expander,
          loaded_file: main_file,
          warnings: warnings,
          frozen_string_literal: frozen_string_literal?,
        ).transform(used: true)
      if debug == 'p1'
        Pass1.debug_instructions(instructions)
        exit
      end

      main_file.instructions = instructions
      transform_file(main_file, @context)

      @context[:required_ruby_files].each_value do |file_info|
        context =
          @context.update(
            # Each required file gets its own top-level variables,
            # so give a fresh vars hash for each file.
            vars: {
            },
          )
        transform_file(file_info, context)
      end

      main_file.instructions
    end

    def transform_file(file_info, compiler_context)
      { 'p2' => Pass2, 'p3' => Pass3, 'p4' => Pass4 }.each do |short_name, klass|
        WhileInstruction.reset_result_id
        file_info.instructions = klass.new(file_info.instructions, compiler_context:).transform
        if debug == short_name
          klass.debug_instructions(file_info.instructions)
          exit
        end
      end
    end

    def macro_expander
      @macro_expander ||=
        MacroExpander.new(
          load_path: load_path,
          compiler_options: options,
          log_load_error: options[:log_load_error],
          compiler_context: @context,
        )
    end
  end
end

# Bundle selectable backends in the self-hosted compiler. MRI loads them on demand.
if RUBY_ENGINE == 'natalie'
  require_relative 'compiler/backends/cpp_backend'
  require_relative 'compiler/backends/llvm_backend'
end
