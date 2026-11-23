require 'tmpdir'
require_relative 'llvm_backend/ir'

module Natalie
  class Compiler
    class LlvmBackend
      ROOT_DIR = File.expand_path('../../../..', __dir__)
      RUNTIME_OBJECTS = %w[llvm_runtime.o llvm_main.o].map { |name| File.join(ROOT_DIR, 'build', name) }.freeze

      def initialize(instructions, compiler:, compiler_context:)
        @instructions = instructions
        @compiler = compiler
        @compiler_context = compiler_context
        raise CompileError, 'LLVM backend requires a 64-bit host' unless [0].pack('J').bytesize == 8
        if compiler.repl? || compiler.compilation_type == 'shared-object' || compiler.build_dir
          raise CompileError,
                'LLVM proof of concept supports scripts and object files; REPL, shared objects, and --build-dir are not supported'
        end
        if compiler_context[:required_cpp_files].any?
          raise CompileError, 'LLVM proof of concept does not yet support inline C++ files'
        end
        @compiler_context.merge!(compile_cxx_flags: [], compile_ld_flags: [])
      end

      def ir
        @ir ||=
          IR.new(
            @instructions,
            source_path: @compiler_context.fetch(:source_path),
            required_files: @compiler_context.fetch(:required_ruby_files),
          ).generate
      end

      def compile_to_object_file(path)
        require_relative 'llvm_backend/native'
        Native.new.emit_object(ir, path)
      end

      def compile_to_binary
        missing = RUNTIME_OBJECTS.reject { |path| File.file?(path) }
        raise CompileError, 'Build the LLVM runtime bridge first: bundle exec rake llvm_runtime' unless missing.empty?
        Dir.mktmpdir('natalie-llvm') do |dir|
          object = File.join(dir, 'program.o')
          compile_to_object_file(object)
          CppBackend::Linker.new(
            in_paths: [object, *RUNTIME_OBJECTS],
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
        [file.path]
      end

      def profile_app!
        raise CompileError, 'LLVM backend does not yet support --profile-app'
      end

      def print_objects!
        raise CompileError, 'LLVM backend does not yet support --print-objects'
      end
    end
  end
end
