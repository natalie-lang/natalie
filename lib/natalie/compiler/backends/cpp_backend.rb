require_relative './cpp_backend/linker'
require_relative './cpp_backend/out_file'
require_relative './cpp_backend/transform'
require_relative '../flags'

module Natalie
  class Compiler
    class CppBackend
      include Flags

      class TransformData
        def initialize(var_prefix: nil)
          @top = {}
          @symbols = {}
          @interned_strings = {}
          @inline_functions = {}
          @var_prefix = var_prefix
          @var_num = 0
        end

        def next_var_num
          @var_num += 1
        end

        attr_reader :top, :symbols, :interned_strings, :inline_functions, :var_prefix
      end

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
      LIBRARIES_FOR_DYNAMIC_LINKING = %w[-lnatalie_base -lonigmo].freeze

      # When compiling a binary with the `-c` option, we use static linking for compatibility.
      LIBRARIES_FOR_STATIC_LINKING = %w[-lnatalie].freeze

      LIB_PATHS = [BUILD_DIR, File.join(BUILD_DIR, 'onigmo/lib'), File.join(BUILD_DIR, 'zlib')].freeze

      def initialize(instructions, compiler:, compiler_context:)
        @instructions = instructions
        @compiler = compiler
        @compiler_context = compiler_context
        augment_compiler_context
        @transform_data = TransformData.new(var_prefix: build_var_prefix)
        @compiled_files = {}
        @var_prefixes_used = {}
      end

      attr_reader :cpp_path, :compiler_context, :compiled_files, :symbols, :interned_strings

      def compile_to_binary
        check_build
        outs = prepare_out_files
        if single_source?
          one_out = merge_out_file_sources(outs)
          object_path = one_out.compile_object_file
          link([object_path])
        else
          object_paths =
            outs.map do |out|
              build_print "compiling #{out.relative_ruby_path}... "
              object_path = out.compile_object_file
              case out.status
              when :compiled
                build_puts 'done'
              when :unchanged
                build_puts 'unchanged'
              else
                raise "unexpected status: #{out.status}"
              end
              object_path
            end
          build_puts 'linking...'
          link(object_paths)
          build_puts 'done'
        end
      end

      def write_files_for_debugging
        outs = prepare_out_files
        if single_source?
          out = merge_out_file_sources(outs)
          [out.write_source_to_tempfile]
        else
          outs.map(&:write_source_to_tempfile)
        end
      end

      def write_object_source(path)
        outs = prepare_out_files
        out = merge_out_file_sources(outs)
        out.write_source_to_path(path)
      end

      def obj_name
        @compiler.write_obj_source_path.sub(/\.rb\.cpp/, '').sub(%r{.*build/(generated/)?}, '').tr('/', '_')
      end

      private

      def merge_out_file_sources(outs)
        main_out = outs.shift
        outs.each { |out| main_out.append_loaded_file(out) }
        main_out
      end

      def prepare_out_files
        outs = []
        outs << prepare_main_out_file
        @compiler_context[:required_ruby_files].each do |name, loaded_file|
          reset_data_for_next_loaded_file(loaded_file)
          outs << prepare_loaded_out_file(name, loaded_file)
        end
        outs
      end

      def reset_data_for_next_loaded_file(loaded_file)
        return if single_source?

        var_prefix = loaded_file.relative_path.sub(/^[^a-zA-Z_]/, '_').gsub(/[^a-zA-Z0-9_]/, '_') + '_'
        if @var_prefixes_used[var_prefix]
          # This causes some hard-to-debug compilation/linking bugs.
          p(var_prefix:, path: loaded_file.path, previous_path: @var_prefixes_used[var_prefix].path)
          raise "I don't know what to do when two var_prefixes collide."
        end
        @var_prefixes_used[var_prefix] = loaded_file

        @transform_data = TransformData.new(var_prefix:)
      end

      def prepare_main_out_file
        main_transform = build_transform(@instructions)
        body = main_transform.transform('return')
        type = write_object_file? ? :obj : :main
        out_file_for_source(type:, body:, ruby_path: @compiler_context[:source_path])
      end

      def prepare_loaded_out_file(name, loaded_file)
        transform = build_transform(loaded_file.instructions)
        body = transform.transform
        out_file_for_source(type: :loaded_file, body:, ruby_path: name)
      end

      def single_source?
        !@compiler.build_dir
      end

      def check_build
        return if File.file?(File.join(BUILD_DIR, "libnatalie_base.#{DL_EXT}"))

        puts 'please run: rake'
        exit 1
      end

      def out_file_for_source(type:, body:, ruby_path:)
        OutFile.new(type:, body:, transform_data: @transform_data, ruby_path:, compiler: @compiler, backend: self)
      end

      def build_transform(instructions)
        Transform.new(
          instructions,
          compiler_context: @compiler_context,
          transform_data: @transform_data,
          compiled_files: @compiled_files,
        )
      end

      def build_var_prefix
        if write_object_file?
          "#{obj_name}_"
        elsif @compiler.repl?
          "repl#{@compiler.repl_num}_"
        else
          nil
        end
      end

      def write_object_file?
        !!@compiler.write_obj_source_path
      end

      def augment_compiler_context
        @compiler_context.merge!(compile_cxx_flags: [], compile_ld_flags: [])
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

      def build_puts(str)
        puts str unless @compiler.build_quietly?
      end

      def build_print(str)
        print str unless @compiler.build_quietly?
      end
    end
  end
end
