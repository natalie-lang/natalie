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
        raise CompileError, 'LLVM backend does not support REPL' if compiler.repl?
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
        Dir.mktmpdir('natalie-llvm-object') do |dir|
          ruby_object = File.join(dir, 'ruby.o')
          native_object = File.join(dir, 'native.o')
          Native.new.emit_object(source, ruby_object)
          @native_extensions.compile(native_object)
          @native_extensions.merge_objects([ruby_object, native_object], path)
        end
      end

      def compile_to_binary
        runtime_objects = shared_library? ? [RUNTIME_BRIDGE] : RUNTIME_OBJECTS
        missing = runtime_objects.reject { |path| File.file?(path) }
        raise CompileError, 'Build the LLVM runtime bridge first: bundle exec rake llvm_runtime' unless missing.empty?
        return build_cache.lock { link([*cached_objects, *runtime_objects]) } if @compiler.build_dir
        Dir.mktmpdir('natalie-llvm') do |dir|
          object = File.join(dir, 'program.o')
          require_relative 'llvm_backend/native'
          Native.new.emit_object(ir, object)
          objects = [object, *runtime_objects]
          if @native_extensions.active? || shared_library?
            native_object = File.join(dir, 'native.o')
            initializer = CppBackend.object_name(@compiler.out_path) if shared_library?
            @native_extensions.compile(native_object, library_initializer: initializer)
            objects << native_object
          end
          link(objects)
        end
      end

      def write_files_for_debugging
        source = ir
        paths =
          [source, *(@units&.sources&.values || [])].map do |unit|
            file = Tempfile.create(%w[natalie .ll])
            file.write(unit)
            file.close
            file.path
          end
        if @native_extensions.active? || shared_library?
          out = @native_extensions.out_file
          out.library_initializer = CppBackend.object_name(@compiler.out_path) if shared_library?
          paths << out.write_source_to_tempfile
        end
        paths
      end

      def profile_app!
        raise CompileError, 'LLVM backend does not yet support --profile-app'
      end

      def print_objects!
        raise CompileError, 'LLVM backend does not yet support --print-objects'
      end

      private

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
        if @native_extensions.active? || shared_library?
          out = @native_extensions.out_file
          out.library_initializer = CppBackend.object_name(@compiler.out_path) if shared_library?
          objects << build_cache.native_object(out)
        end
        objects
      end

      def link(objects)
        puts 'linking...' if @compiler.build_dir && !@compiler.build_quietly?
        CppBackend::Linker.new(
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
    end
  end
end
