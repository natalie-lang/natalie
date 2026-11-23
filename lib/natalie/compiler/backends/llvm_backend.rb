require 'tmpdir'
require_relative 'llvm_backend/ir'
require_relative 'llvm_backend/native_extensions'
require_relative 'llvm_backend/units'
require_relative 'llvm_backend/build_cache'

module Natalie
  class Compiler
    class LlvmBackend
      ROOT_DIR = File.expand_path('../../../..', __dir__)
      RUNTIME_BRIDGE = File.join(ROOT_DIR, 'build/llvm_runtime.o').freeze
      RUNTIME_OBJECTS = [RUNTIME_BRIDGE, File.join(ROOT_DIR, 'build/llvm_main.o')].freeze

      def initialize(instructions, compiler:, compiler_context:)
        @instructions = instructions
        @compiler = compiler
        @compiler_context = compiler_context
        raise CompileError, 'LLVM backend requires a 64-bit host' unless [0].pack('J').bytesize == 8
        @compiler_context.merge!(compile_cxx_flags: [], compile_ld_flags: [])
        @native_extensions = NativeExtensions.new(compiler: compiler, backend: self)
      end

      attr_reader :compiler_context

      def ir
        @ir ||=
          IR.new(
            @instructions,
            source_path: @compiler_context.fetch(:source_path),
            required_files: @compiler_context.fetch(:required_ruby_files),
            native_extensions: @native_extensions,
            shared_library: shared_library?,
            repl: @compiler.repl?,
            units: units,
          ).generate
      end

      def compile_to_object_file(path)
        require_relative 'llvm_backend/native'
        if @compiler.build_dir
          return(
            build_cache.lock do
              objects = cached_objects
              if objects.size == 1
                FileUtils.cp(objects.first, path)
              else
                @native_extensions.merge_objects(objects, path)
              end
            end
          )
        end
        source = ir
        return Native.new.emit_object(source, path) unless @native_extensions.active?
        Dir.mktmpdir do |dir|
          ruby_object = File.join(dir, 'ruby.o')
          native_object = File.join(dir, 'native.o')
          Native.new.emit_object(source, ruby_object)
          @native_extensions.compile(native_object)
          @native_extensions.merge_objects([ruby_object, native_object], path)
        end
      end

      def compile_to_binary
        if @compiler.options[:keep_source] && !@compiler.build_dir
          write_files_for_debugging.each { |path| puts "source file path is: #{path}" }
        end
        runtime_objects = library? || @custom_entrypoint ? [RUNTIME_BRIDGE] : RUNTIME_OBJECTS
        missing = runtime_objects.reject { |path| File.file?(path) }
        raise CompileError, 'Build the LLVM runtime bridge first: bundle exec rake llvm_runtime' unless missing.empty?
        if @compiler.build_dir
          return(
            build_cache.lock do
              objects = [*cached_objects, *runtime_objects]
              objects << build_cache.native_object(entrypoint, name: 'entrypoint') if @custom_entrypoint && !library?
              link(objects)
            end
          )
        end
        Dir.mktmpdir do |dir|
          object = File.join(dir, 'program.o')
          require_relative 'llvm_backend/native'
          Native.new.emit_object(ir, object)
          objects = [object, *runtime_objects]
          if @custom_entrypoint && !library?
            main = entrypoint
            main.out_path = File.join(dir, 'entrypoint.o')
            objects << main.compile_object_file
          end
          if @native_extensions.active? || library?
            native_object = File.join(dir, 'native.o')
            initializer = NativeBuild.object_name(@compiler.out_path) if shared_library?
            @native_extensions.compile(native_object, library_initializer: initializer)
            objects << native_object
          end
          link(objects)
        end
      end

      def write_files_for_debugging
        source = ir
        sources = [[@compiler_context.fetch(:source_path), source], *(@units&.sources || {}).to_a]
        paths =
          sources.map do |source_path, unit|
            file = Tempfile.create([File.basename(source_path), '.ll'])
            file.write(unit)
            file.close
            file.path
          end
        if @native_extensions.active? || library?
          out = @native_extensions.out_file
          out.library_initializer = NativeBuild.object_name(@compiler.out_path) if shared_library?
          paths << out.write_source_to_tempfile
        end
        paths
      end

      def profile_app!
        @compiler_context.fetch(:compile_cxx_flags) << '-DNAT_NATIVE_PROFILER'
        @custom_entrypoint = true
      end

      def print_objects!
        @compiler_context.fetch(:compile_cxx_flags) << '-DNAT_PRINT_OBJECTS'
        @custom_entrypoint = true
      end

      private

      def entrypoint
        data = NativeBuild::TransformData.new
        data.top[:llvm] = '#include "natalie/llvm_runtime.hpp"'
        # Native dependencies initialize in the LLVM sidecar, not twice here.
        context = @compiler_context.merge(required_cpp_files: {})
        backend = Struct.new(:compiled_files, :compiler_context).new({}, context)
        NativeBuild::SourceFile.new(
          type: :main,
          body: 'return nat_llvm_unpack(nat_llvm_eval(env, nat_llvm_pack(self)));',
          transform_data: data,
          ruby_path: @compiler_context.fetch(:source_path),
          compiler: @compiler,
          backend: backend,
        )
      end

      def units
        return unless @compiler.build_dir

        @units ||=
          Units.new(
            required_files: @compiler_context.fetch(:required_ruby_files),
            native_extensions: @native_extensions,
          )
      end

      def build_cache
        @build_cache ||= BuildCache.new(@compiler, @compiler_context.fetch(:source_path))
      end

      def cached_objects
        require_relative 'llvm_backend/native'
        main_source = ir # Recursively generates the independent loaded-file units.
        native = Native.new
        objects = [build_cache.llvm_object('main', main_source, native)]
        units.sources.each { |path, source| objects << build_cache.llvm_object(Units.id(path), source, native) }
        if @native_extensions.active? || library?
          out = @native_extensions.out_file
          out.library_initializer = NativeBuild.object_name(@compiler.out_path) if shared_library?
          objects << build_cache.native_object(out)
        end
        objects
      end

      def link(objects)
        puts 'linking...' if @compiler.build_dir && !@compiler.build_quietly?
        NativeBuild::Linker.new(
          in_paths: objects,
          out_path: @compiler.out_path,
          compiler: @compiler,
          compiler_context: @compiler_context,
          keep_inputs: true,
        ).link
        puts 'done' if @compiler.build_dir && !@compiler.build_quietly?
      end

      def shared_library?
        @compiler.compilation_type == 'shared-object'
      end

      def library?
        shared_library? || @compiler.repl?
      end
    end
  end
end
