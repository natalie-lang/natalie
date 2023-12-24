require 'natalie/inline'
require 'ffi.cpp'

__ld_flags__ '-lffi -ldl'

module FFI
  class NotFoundError < LoadError; end

  module Library
    __bind_method__ :ffi_lib, :FFI_Library_ffi_lib
    __bind_method__ :attach_function, :FFI_Library_attach_function
  end

  class DynamicLibrary
    def initialize(name, lib)
      @name = name
      @lib = lib
    end

    attr_reader :name
  end

  class Pointer
    __bind_method__ :address, :FFI_Pointer_address
    __bind_method__ :autorelease?, :FFI_Pointer_is_autorelease
    __bind_method__ :autorelease=, :FFI_Pointer_set_autorelease
    __bind_method__ :initialize, :FFI_Pointer_initialize
    __bind_method__ :read_string, :FFI_Pointer_read_string
    __bind_method__ :free, :FFI_Pointer_free

    attr_reader :type_size

    def null?
      address == 0
    end

    def ==(other)
      case other
      when nil
        null?
      when Pointer
        address == other.address
      else
        false
      end
    end
  end

  class MemoryPointer < Pointer
    __bind_method__ :initialize, :FFI_MemoryPointer_initialize
    __bind_method__ :inspect, :FFI_MemoryPointer_inspect
  end
end
