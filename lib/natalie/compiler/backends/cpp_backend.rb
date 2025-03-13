require_relative './cpp_backend/linker'
require_relative './cpp_backend/out_file'
require_relative './cpp_backend/transform'
require_relative '../flags'

module Natalie
  class Compiler
    class CppBackend
      include Flags

      ROOT_DIR = File.expand_path('../../../../', __dir__)
      BUILD_DIR = File.join(ROOT_DIR, 'build')

      DARWIN = RUBY_PLATFORM.match?(/darwin/)
      OPENBSD = RUBY_PLATFORM.match?(/openbsd/)

      DL_EXT = RbConfig::CONFIG['DLEXT']

      INC_PATHS = [
        File.join(ROOT_DIR, 'include'),
        File.join(ROOT_DIR, 'ext/tm/include'),
        File.join(BUILD_DIR),
        File.join(BUILD_DIR, 'onigmo/include'),
      ].freeze

      # When running `bin/natalie script.rb`, we use dynamic linking to speed things up.
      LIBRARIES_FOR_DYNAMIC_LINKING = %w[
        -lnatalie_base
        -lonigmo
      ].freeze

      # When compiling a binary with the `-c` option, we use static linking for compatibility.
      LIBRARIES_FOR_STATIC_LINKING = %w[
        -lnatalie
      ].freeze

      LIB_PATHS = [
        BUILD_DIR,
        File.join(BUILD_DIR, 'onigmo/lib'),
        File.join(BUILD_DIR, 'zlib'),
      ].freeze

      def initialize(instructions, compiler:, compiler_context:)
        @instructions = instructions
        @compiler = compiler
        @compiler_context = compiler_context
        augment_compiler_context
        @symbols = {}
        @interned_strings = {}
        @inline_functions = {}
        @compiled_files = {}
        check_build
      end

      attr_reader :cpp_path, :compiler_context, :compiled_files, :symbols, :interned_strings

      def compile_to_binary
        outs = prepare_out_files
        if ENV['SINGLE_SOURCE']
          main_out = outs.shift
          outs.each { |out| main_out.append_loaded_file(out) }
          object_path = main_out.compile_object_file
          link([object_path])
        else
          object_paths = outs.map do |out|
            puts "compiling #{out.ruby_path}..."
            out.compile_object_file
          end
          puts 'linking...'
          link(object_paths)
          puts 'done'
        end
      end

      def write_files_for_debugging
        prepare_out_files.map { |out| out.write_source_to_tempfile }
      end

      # def compile_to_binary
      #   prepare_temp
      #   compile_temp_to_binary
      # end
      #
      # def prepare_temp
      #   check_build
      #   write_file
      # end
      #
      # def compile_temp_to_binary
      #   object_file_path = @out_file.compile_object_file
      #   link([object_file_path])
      # end
      #
      # def write_object_source(path)
      #   @out_file = build_main_out_file
      #   @out_file.write_source_to_path(path)
      # end
      #
      # def write_file_for_debugging
      #   write_file
      #   @cpp_path
      # end
      #
      # def compiler_command
      #   @out_file = build_main_out_file
      #   @out_file.compiler_command
      #   [
      #     @out_file.compiler_command,
      #     linker([@out_file.out_path]).link_command,
      #   ].join("\n")
      # end

      private

      def prepare_out_files
        @top = {}
        outs = []
        outs << prepare_main_out_file
        @compiler_context[:required_ruby_files].each do |name, loaded_file|
          @top = {} if reset_top_for_each_file?
          outs << prepare_loaded_out_file(name, loaded_file)
        end
        outs
      end

      def prepare_main_out_file
        main_transform = build_transform(@instructions)
        body = main_transform.transform('return')
        type = write_object_file? ? :obj : :main
        out_file_for_source(type:, body:, ruby_path: @compiler_context[:source_path])
      end

      def prepare_loaded_out_file(name, loaded_file)
        transform = build_transform(loaded_file.instructions)
        body = transform.transform('return')
        out_file_for_source(type: :loaded_file, body:, ruby_path: name)
      end

      def reset_top_for_each_file?
        !ENV['SINGLE_SOURCE']
      end

      # def write_file
      #   @out_file = build_main_out_file
      #   @cpp_path = @out_file.write_source_to_tempfile
      # end

      def check_build
        return if File.file?(File.join(BUILD_DIR, "libnatalie_base.#{DL_EXT}"))

        puts 'please run: rake'
        exit 1
      end

      def out_file_for_source(type:, body:, ruby_path:)
        OutFile.new(
          type:,
          body:,
          top: @top,
          ruby_path:,
          compiler: @compiler,
          backend: self,
        )
      end

      def build_transform(instructions)
        Transform.new(
          instructions,
          top:              @top,
          compiler_context: @compiler_context,
          symbols:          @symbols,
          interned_strings: @interned_strings,
          inline_functions: @inline_functions,
          compiled_files:   @compiled_files,
        )
      end

      def obj_name
        @compiler
          .write_obj_source_path
          .sub(/\.rb\.cpp/, '')
          .sub(%r{.*build/(generated/)?}, '')
          .tr('/', '_')
      end

      def var_prefix
        if write_object_file?
          "#{obj_name}_"
        elsif @compiler.repl?
          "repl#{@compiler.repl_num}_"
        else
          ''
        end
      end

      def write_object_file?
        !!@compiler.write_obj_source_path
      end

      def augment_compiler_context
        @compiler_context.merge!(
          compile_cxx_flags: [],
          compile_ld_flags:  [],
          var_prefix:        var_prefix,
        )
      end

      def linker(paths)
        Linker.new(
          in_paths: paths,
          out_path: @compiler.out_path,
          compiler: @compiler,
          compiler_context: @compiler_context,
        )
      end

      def link(paths)
        linker(paths).link
      end
    end
  end
end
