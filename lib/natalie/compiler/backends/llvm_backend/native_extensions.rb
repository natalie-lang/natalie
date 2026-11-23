require 'shellwords'

module Natalie
  class Compiler
    class LlvmBackend
      # Reuse the C++ inline-macro generator for native code only. Ruby bodies
      # stay in LLVM; the generated wrappers pass tagged words across the ABI.
      class NativeExtensions
        class OutFile < CppBackend::OutFile
          attr_accessor :library_initializer
          attr_accessor :dependency_file
          attr_writer :cpp_path

          def compiler_command
            command = super
            command += " -MMD -MF #{dependency_file.shellescape}" if dependency_file
            command
          end

          def merged_source
            <<~CPP
              #include "natalie/llvm_runtime.hpp"
              using namespace Natalie;
              #{declarations}
              extern "C" __attribute__((visibility("hidden"))) void nat_llvm_init_extensions(Env *env, uint64_t self_bits) {
                  auto self = nat_llvm_unpack(self_bits);
                  #{init_matter}
                  #{@body}
              }
              #{library_entrypoint}
            CPP
          end

          def library_entrypoint
            return '' unless library_initializer

            <<~CPP
              extern "C" __attribute__((visibility("hidden"))) uint64_t nat_llvm_eval(Env *, uint64_t);
              Value init_#{library_initializer}(Env *env, Value self) {
                  return nat_llvm_unpack(nat_llvm_eval(env, nat_llvm_pack(self)));
              }
            CPP
          end
        end

        def initialize(compiler:, backend:)
          @compiler = compiler
          @backend = backend
          prefix = 'llvm_ext_'
          prefix += "#{CppBackend.object_name(compiler.out_path)}_" if compiler.compilation_type == 'shared-object'
          @data = CppBackend::TransformData.new(var_prefix: prefix)
          @wrappers = []
          @file_wrapper_counts = Hash.new(0)
        end

        def active?
          @wrappers.any?
        end

        def declarations(wrappers: nil, include_init: true)
          declarations = include_init && active? ? ['declare void @nat_llvm_init_extensions(ptr, i64)'] : []
          declarations.concat((wrappers || @wrappers).map { |name| "declare i64 @#{name}(ptr, i64, ptr)" }).join("\n")
        end

        def wrapper(instruction, argument_count, source_path: nil)
          if source_path
            index = @file_wrapper_counts[source_path]
            @file_wrapper_counts[source_path] += 1
            name = "nat_llvm_ext_#{Units.id(source_path)}_#{index}"
          else
            name = "nat_llvm_ext_#{@wrappers.size}"
          end
          stack = Array.new(argument_count) { |i| "nat_llvm_unpack(argv[#{i}])" }
          transform =
            CppBackend::Transform.new(
              [instruction],
              compiler_context: @backend.compiler_context,
              transform_data: @data,
              stack: stack,
            )
          body = transform.transform('return')
          @data.top[name] = <<~CPP
            extern "C" __attribute__((visibility("hidden"))) uint64_t #{name}(Env *env, uint64_t self_bits, const uint64_t *argv) {
                auto self = nat_llvm_unpack(self_bits);
                auto result = [&]() -> Value {
                    #{body}
                }();
                return nat_llvm_pack(result);
            }
          CPP
          @wrappers << name
          name
        end

        def out_file
          init = @backend.compiler_context.fetch(:required_cpp_files).values.map { |name| "init_#{name}(env, self);" }
          OutFile.new(
            type: :obj,
            body: init.join("\n"),
            transform_data: @data,
            ruby_path: @backend.compiler_context.fetch(:source_path),
            compiler: @compiler,
            backend: @backend,
          )
        end

        def compile(path, library_initializer: nil)
          out = out_file
          out.library_initializer = library_initializer
          out.out_path = path
          out.compile_object_file
        end

        def merge_objects(paths, output)
          command = [*Shellwords.split(ENV.fetch('CXX', 'c++')), '-r', '-nostdlib']
          command << '-no-pie' unless CppBackend::DARWIN
          command.concat([*paths, '-o', output])
          puts command.shelljoin if @compiler.debug == 'cc-cmd'
          output = `#{command.shelljoin} 2>&1`
          status = $?
          raise CompileError, "Could not combine LLVM and native extension objects:\n#{output}" unless status.success?
        end
      end
    end
  end
end
