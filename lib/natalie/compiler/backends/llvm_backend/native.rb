require 'fiddle/import'
require 'open3'

module Natalie
  class Compiler
    class LlvmBackend
      # Keep LLVM behind its C API so the Ruby compiler needs no compiled extension.
      class Native
        SIGNATURES = [
          'void* LLVMContextCreate()',
          'void LLVMContextDispose(void*)',
          'void* LLVMCreateMemoryBufferWithMemoryRangeCopy(void*, size_t, char*)',
          'int LLVMParseIRInContext(void*, void*, void*, void*)',
          'int LLVMVerifyModule(void*, int, void*)',
          'void LLVMDisposeModule(void*)',
          'void LLVMDisposeMessage(void*)',
          'void* LLVMGetDefaultTargetTriple()',
          'int LLVMGetTargetFromTriple(char*, void*, void*)',
          'void* LLVMCreateTargetMachine(void*, char*, char*, char*, int, int, int)',
          'void LLVMDisposeTargetMachine(void*)',
          'void LLVMSetTarget(void*, char*)',
          'void* LLVMCreateTargetDataLayout(void*)',
          'void* LLVMCopyStringRepOfTargetData(void*)',
          'void LLVMSetDataLayout(void*, char*)',
          'void LLVMDisposeTargetData(void*)',
          'int LLVMTargetMachineEmitToFile(void*, void*, char*, int, void*)',
        ].freeze

        def initialize
          @api = Module.new { extend Fiddle::Importer }
          @api.dlload(library)
          SIGNATURES.each { |signature| @api.extern(signature) }
        rescue Fiddle::DLError => e
          raise CompileError,
                "Cannot load LLVM: #{e.message}. Set NAT_LLVM_LIBRARY to a shared LLVM library (16 or newer)."
        end

        def emit_object(ir, path)
          context = @api.LLVMContextCreate
          mod_out = pointer_slot
          error = pointer_slot
          buffer = @api.LLVMCreateMemoryBufferWithMemoryRangeCopy(ir, ir.bytesize, 'natalie')
          check(@api.LLVMParseIRInContext(context, buffer, mod_out, error), error)
          mod = read_pointer(mod_out)
          # LLVMReturnStatusAction: report malformed IR without aborting the host Ruby.
          check(@api.LLVMVerifyModule(mod, 2, error), error)
          triple = @api.LLVMGetDefaultTargetTriple
          target_triple = triple.to_s
          if RUBY_PLATFORM.include?('darwin') && ENV['MACOSX_DEPLOYMENT_TARGET']
            target_triple =
              target_triple.sub(
                /-apple-(?:macosx|darwin)[\d.]*/,
                "-apple-macosx#{ENV.fetch('MACOSX_DEPLOYMENT_TARGET')}",
              )
          end
          arch =
            case triple.to_s
            when /\A(aarch64|arm64)/
              'AArch64'
            when /\A(x86_64|i[3-6]86)/
              'X86'
            else
              raise CompileError, "Unsupported LLVM host: #{triple}"
            end
          %w[TargetInfo Target TargetMC AsmPrinter].each do |component|
            name = "LLVMInitialize#{arch}#{component}"
            @api.extern("void #{name}()")
            @api.send(name)
          end
          target_out = pointer_slot
          check(@api.LLVMGetTargetFromTriple(target_triple, target_out, error), error)
          # No optimization passes yet; PIC permits both normal and shared linking.
          machine = @api.LLVMCreateTargetMachine(read_pointer(target_out), target_triple, 'generic', '', 0, 2, 0)
          raise CompileError, 'LLVM could not create a target machine' if machine.null?
          @api.LLVMSetTarget(mod, target_triple)
          layout = @api.LLVMCreateTargetDataLayout(machine)
          layout_string = @api.LLVMCopyStringRepOfTargetData(layout)
          @api.LLVMSetDataLayout(mod, layout_string)
          check(@api.LLVMTargetMachineEmitToFile(machine, mod, path, 1, error), error)
        ensure
          @api.LLVMDisposeMessage(layout_string) if layout_string
          @api.LLVMDisposeTargetData(layout) if layout
          @api.LLVMDisposeTargetMachine(machine) if machine && !machine.null?
          @api.LLVMDisposeMessage(triple) if triple
          @api.LLVMDisposeModule(mod) if mod
          @api.LLVMContextDispose(context) if context
        end

        private

        def library
          return ENV.fetch('NAT_LLVM_LIBRARY') if ENV['NAT_LLVM_LIBRARY']

          begin
            dir, status = Open3.capture2('llvm-config', '--libdir')
            if status.success?
              candidate = Dir[File.join(dir.strip, 'libLLVM.{so,dylib}'), File.join(dir.strip, 'libLLVM-*.so')].first
              return candidate if candidate
            end
          rescue Errno::ENOENT
          end
          RUBY_PLATFORM.include?('darwin') ? 'libLLVM.dylib' : 'libLLVM.so'
        end

        def pointer_slot
          Fiddle::Pointer[Array.new(Fiddle::SIZEOF_VOIDP, 0).pack('C*')]
        end

        def read_pointer(slot)
          Fiddle::Pointer.new(slot[0, Fiddle::SIZEOF_VOIDP].unpack1('J'))
        end

        def check(status, error)
          message = read_pointer(error)
          text = message.null? ? 'unknown LLVM error' : message.to_s
          @api.LLVMDisposeMessage(message) unless message.null?
          error[0, Fiddle::SIZEOF_VOIDP] = [0].pack('J')
          raise CompileError, text unless status.zero?
        end
      end
    end
  end
end
