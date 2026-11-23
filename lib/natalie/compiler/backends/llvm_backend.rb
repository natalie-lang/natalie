require 'tmpdir'
require_relative 'llvm_backend/ir'
require_relative 'llvm_backend/native_extensions'

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
        raise CompileError, 'LLVM backend does not support REPL or --build-dir' if compiler.repl? || compiler.build_dir
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
          ).generate
      end

      def compile_to_object_file(path)
        require_relative 'llvm_backend/native'
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
          CppBackend::Linker.new(
            in_paths: objects,
            out_path: @compiler.out_path,
            compiler: @compiler,
            compiler_context: @compiler_context,
            keep_inputs: true,
          ).link
        end
      end

      def write_files_for_debugging
        file = Tempfile.create(%w[natalie .ll])
        file.write(ir)
        file.close
        paths = [file.path]
        paths << @native_extensions.out_file.write_source_to_tempfile if @native_extensions.active?
        paths
      end

      def profile_app!
        raise CompileError, 'LLVM backend does not yet support --profile-app'
      end

      def print_objects!
        raise CompileError, 'LLVM backend does not yet support --print-objects'
      end

      private

      def shared_library?
        @compiler.compilation_type == 'shared-object'
      end
    end
  end
end
