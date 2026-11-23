if RUBY_ENGINE == 'natalie'
  require_relative 'native_ffi'
else
  # Keep MRI's gem outside Natalie's static require expansion while retaining
  # RubyGems' require hook (Kernel.require bypasses gem activation).
  Object.new.send(:require, 'fiddle/import')
end

module Natalie
  class Compiler
    class LlvmBackend
      # Keep LLVM behind its C API so the Ruby compiler needs no compiled extension.
      class Native
        API_ERROR = RUBY_ENGINE == 'natalie' ? LoadError : Fiddle::DLError
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
          'void* LLVMCreatePassBuilderOptions()',
          'void LLVMDisposePassBuilderOptions(void*)',
          'void* LLVMRunPasses(void*, char*, void*, void*)',
          'void* LLVMGetErrorMessage(void*)',
          'void LLVMDisposeErrorMessage(void*)',
          'void* LLVMGetFirstFunction(void*)',
          'void* LLVMGetNextFunction(void*)',
          'int LLVMIsDeclaration(void*)',
          'int LLVMGetEnumAttributeKindForName(char*, size_t)',
          'void* LLVMCreateEnumAttribute(void*, int, size_t)',
          'void LLVMAddAttributeAtIndex(void*, int, void*)',
          'void LLVMAddTargetDependentFunctionAttr(void*, char*, char*)',
          'int LLVMTargetMachineEmitToFile(void*, void*, char*, int, void*)',
        ].freeze

        def initialize(build: nil)
          @optimization = build == 'release' ? 3 : 0
          @sanitize_address = build == 'sanitized'
          if @sanitize_address && ENV.fetch('NAT_SANITIZE_FLAG_VALUE', 'address') != 'address'
            raise CompileError, 'LLVM instrumentation currently supports NAT_SANITIZE_FLAG_VALUE=address only'
          end
          @api =
            if RUBY_ENGINE == 'natalie'
              Module.new { extend NativeFFI }
            else
              Module.new { extend Fiddle::Importer }
            end
          @library = library
          @api.dlload(@library)
          SIGNATURES.each { |signature| @api.extern(signature) }
        rescue API_ERROR => e
          raise CompileError,
                "Cannot load LLVM: #{e.message}. Set NAT_LLVM_LIBRARY to a shared LLVM library (16 or newer)."
        end

        def cache_key
          @cache_key ||=
            begin
              identity =
                if File.file?(@library)
                  stat = File.stat(@library)
                  [File.realpath(@library), stat.size, stat.mtime.to_r.to_s]
                else
                  # Some hosts resolve a bare soname through the dynamic loader.
                  # If version discovery is unavailable, conservatively avoid
                  # reusing objects from a different compiler process.
                  begin
                    @api.extern('void LLVMGetVersion(void*, void*, void*)')
                    version = Array.new(3) { pointer_slot }
                    @api.LLVMGetVersion(*version)
                    [@library, version.map { |part| pointer_bytes(part, 4).unpack1('I') }]
                  rescue API_ERROR
                    [@library, Process.pid]
                  end
                end
              [
                identity,
                RUBY_PLATFORM,
                ENV['MACOSX_DEPLOYMENT_TARGET'],
                @optimization,
                @sanitize_address,
                File.binread(__FILE__),
              ]
            end
        end

        def emit_object(ir, path)
          emit_file(ir, path, 1) # LLVMObjectFile
        end

        def emit_assembly(ir, path)
          emit_file(ir, path, 0) # LLVMAssemblyFile
        end

        private

        def emit_file(ir, path, file_type)
          context = @api.LLVMContextCreate
          mod_out = pointer_slot
          error = pointer_slot
          buffer = @api.LLVMCreateMemoryBufferWithMemoryRangeCopy(ir, ir.bytesize, 'natalie')
          check(@api.LLVMParseIRInContext(context, buffer, mod_out, error), error)
          mod = read_pointer(mod_out)
          # LLVMReturnStatusAction: report malformed IR without aborting the host Ruby.
          check(@api.LLVMVerifyModule(mod, 2, error), error)
          triple = @api.LLVMGetDefaultTargetTriple
          target_triple = pointer_string(triple)
          if RUBY_PLATFORM.include?('darwin') && ENV['MACOSX_DEPLOYMENT_TARGET']
            target_triple =
              target_triple.sub(
                /-apple-(?:macosx|darwin)[\d.]*/,
                "-apple-macosx#{ENV.fetch('MACOSX_DEPLOYMENT_TARGET')}",
              )
          end
          arch =
            case target_triple
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
          # PIC permits both normal and shared linking.
          machine =
            @api.LLVMCreateTargetMachine(read_pointer(target_out), target_triple, 'generic', '', @optimization, 2, 0)
          raise CompileError, 'LLVM could not create a target machine' if machine.null?
          @api.LLVMSetTarget(mod, target_triple)
          layout = @api.LLVMCreateTargetDataLayout(machine)
          layout_string = @api.LLVMCopyStringRepOfTargetData(layout)
          @api.LLVMSetDataLayout(mod, layout_string)
          run_passes(mod, machine, "default<O#{@optimization}>") if @optimization > 0
          instrument_address_sanitizer(mod, context, machine) if @sanitize_address
          check(@api.LLVMTargetMachineEmitToFile(machine, mod, path, file_type, error), error)
        ensure
          @api.LLVMDisposeMessage(layout_string) if layout_string
          @api.LLVMDisposeTargetData(layout) if layout
          @api.LLVMDisposeTargetMachine(machine) if machine && !machine.null?
          @api.LLVMDisposeMessage(triple) if triple
          @api.LLVMDisposeModule(mod) if mod
          @api.LLVMContextDispose(context) if context
        end

        private

        def instrument_address_sanitizer(mod, context, machine)
          name = 'sanitize_address'
          kind = @api.LLVMGetEnumAttributeKindForName(name, name.bytesize)
          attribute = @api.LLVMCreateEnumAttribute(context, kind, 0)
          function = @api.LLVMGetFirstFunction(mod)
          until function.null?
            if @api.LLVMIsDeclaration(function) == 0
              @api.LLVMAddAttributeAtIndex(function, -1, attribute) # LLVMAttributeFunctionIndex
              @api.LLVMAddTargetDependentFunctionAttr(function, 'frame-pointer', 'all')
            end
            function = @api.LLVMGetNextFunction(function)
          end
          run_passes(mod, machine, 'asan')
        end

        def run_passes(mod, machine, pipeline)
          options = @api.LLVMCreatePassBuilderOptions
          error = @api.LLVMRunPasses(mod, pipeline, machine, options)
          return if error.null?
          message = @api.LLVMGetErrorMessage(error)
          raise CompileError, "LLVM pass pipeline #{pipeline} failed: #{pointer_string(message)}"
        ensure
          @api.LLVMDisposeErrorMessage(message) if message
          @api.LLVMDisposePassBuilderOptions(options) if options
        end

        def library
          return ENV.fetch('NAT_LLVM_LIBRARY') if ENV['NAT_LLVM_LIBRARY']

          begin
            dir = IO.popen('llvm-config --libdir 2>/dev/null', &:read)
            status = $?
            if status.success?
              candidate =
                %w[libLLVM.so libLLVM.dylib libLLVM-*.so]
                  .flat_map { |pattern| Dir[File.join(dir.strip, pattern)] }
                  .first
              return candidate if candidate
            end
          rescue Errno::ENOENT
          end
          RUBY_PLATFORM.include?('darwin') ? 'libLLVM.dylib' : 'libLLVM.so'
        end

        def pointer_slot
          if RUBY_ENGINE == 'natalie'
            FFI::MemoryPointer.new(:pointer, 1, true)
          else
            Fiddle::Pointer[[0].pack('J')]
          end
        end

        def read_pointer(slot)
          address = pointer_bytes(slot, 8).unpack1('J')
          RUBY_ENGINE == 'natalie' ? FFI::Pointer.new(address) : Fiddle::Pointer.new(address)
        end

        def pointer_bytes(pointer, size)
          RUBY_ENGINE == 'natalie' ? pointer.read_string(size) : pointer[0, size]
        end

        def pointer_string(pointer)
          RUBY_ENGINE == 'natalie' ? pointer.read_string : pointer.to_s
        end

        def check(status, error)
          message = read_pointer(error)
          text = message.null? ? 'unknown LLVM error' : pointer_string(message)
          @api.LLVMDisposeMessage(message) unless message.null?
          if RUBY_ENGINE == 'natalie'
            error.write_string([0].pack('J'))
          else
            error[0, 8] = [0].pack('J')
          end
          raise CompileError, text unless status.zero?
        end
      end
    end
  end
end
