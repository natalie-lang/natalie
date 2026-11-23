require 'ffi'

module Natalie
  class Compiler
    class LlvmBackend
      # Adapt the LLVM C signatures to Natalie's libffi bindings. The emitter
      # uses the same LLVM API and ownership rules under MRI and Natalie.
      module NativeFFI
        def self.extended(api)
          api.extend(FFI::Library)
        end

        def dlload(library)
          ffi_lib(library)
        end

        def extern(signature)
          match = /\A(.+?)\s+(\w+)\((.*)\)\z/.match(signature)
          result, name, arguments = match.captures
          types = arguments.empty? ? [] : arguments.split(',').map { |type| native_type(type.strip) }
          attach_function(name.to_sym, types, native_type(result))
        end

        private

        def native_type(type)
          return :pointer if type.end_with?('*')
          { 'void' => :void, 'int' => :int, 'size_t' => :size_t }.fetch(type)
        end
      end
    end
  end
end
