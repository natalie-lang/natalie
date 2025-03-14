require 'tempfile'
require_relative '../../build/generated/numbers'
require_relative './compiler/backends/cpp_backend'
require_relative './compiler/bytecode/header'
require_relative './compiler/bytecode/loader'
require_relative './compiler/bytecode/ro_data'
require_relative './compiler/bytecode/sections'
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

    def initialize(ast:, path:, encoding: Encoding::UTF_8, warnings: [], data_loc: nil, options: {})
      @ast = ast
      @var_num = 0
      @path = path
      @encoding = encoding
      @warnings = warnings
      @data_loc = data_loc
      @options = options
      @inline_cpp_enabled = {}
    end

    attr_accessor :ast,
                  :warnings,
                  :context,
                  :inline_cpp_enabled,
                  :options,
                  :repl,
                  :repl_num,
                  :vars,
                  :write_obj_source_path

    attr_writer :load_path, :out_path

    def compile
      backend.compile_to_binary
    end

    def write_object_source
      backend.write_object_source(write_obj_source_path)
    end

    def write_bytecode_to_file
      File.open(@out_path, 'wb') do |file|
        compile_to_bytecode(file)
      end
    end

    def compile_to_bytecode(io)
      rodata = Bytecode::RoData.new
      bytecode = instructions.each.with_object(''.b) do |instruction, output|
        output << instruction.serialize(rodata)
      end

      header = Bytecode::Header.new
      io.write(header)

      sections = Bytecode::Sections.new(header:, rodata:, bytecode:)
      io.write(sections)

      unless rodata.empty?
        io.write([rodata.bytesize].pack('N'))
        io.write(rodata)
      end

      # Format of every section: size (32 bits), content
      io.write([bytecode.bytesize].pack('N'))
      io.write(bytecode)
    end

    def write_files_for_debugging
      backend.write_files_for_debugging
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

    def debug = options[:debug]
    def build = options[:build]
    def build_dir = options[:build_dir]
    def keep_cpp? = !!(debug || options[:keep_cpp])
    def interpret? = !!options[:interpret]
    def dynamic_linking? = !!options[:dynamic_linking]
    def repl? = !!repl
    def frozen_string_literal? = !!options[:frozen_string_literal]

    private

    def data_loc
      return nil if @data_loc.nil?

      { path: @path, data_loc: @data_loc }
    end

    def transform
      @context = build_context

      main_file = LoadedFile.new(path: @path, encoding: @encoding)

      instructions = Pass1.new(
        ast,
        compiler_context:      @context,
        data_loc:              data_loc,
        macro_expander:        macro_expander,
        loaded_file:           main_file,
        warnings:              warnings,
        frozen_string_literal: frozen_string_literal?
      ).transform(
        used: true
      )
      if debug == 'p1'
        Pass1.debug_instructions(instructions)
        exit
      end

      main_file.instructions = instructions
      transform_file(main_file, @context)

      @context[:required_ruby_files].each_value do |file_info|
        context = @context.update(
          # Each required file gets its own top-level variables,
          # so give a fresh vars hash for each file.
          vars: {}
        )
        transform_file(file_info, context)
      end

      main_file.instructions
    end

    def transform_file(file_info, compiler_context)
      {
        'p2' => Pass2,
        'p3' => Pass3,
        'p4' => Pass4,
      }.each do |short_name, klass|
        file_info.instructions = klass.new(
          file_info.instructions,
          compiler_context:,
        ).transform
        if debug == short_name
          klass.debug_instructions(file_info.instructions)
          exit
        end
      end
    end

    def macro_expander
      @macro_expander ||= MacroExpander.new(
        load_path:        load_path,
        interpret:        interpret?,
        log_load_error:   options[:log_load_error],
        compiler_context: @context,
      )
    end
  end
end
