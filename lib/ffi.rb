require 'natalie/inline'
require 'ffi.cpp'

__ld_flags__ '-lffi -ldl'

module FFI
  class NotFoundError < LoadError; end

  module Library
    __bind_method__ :ffi_lib, :FFI_Library_ffi_lib
    __bind_method__ :attach_function, :FFI_Library_attach_function

    def callback(name, param_types, return_type)
      @ffi_typedefs ||= {}
      @ffi_typedefs[name] = FFI::FunctionType.new(param_types, return_type)
    end
  end

  class FunctionType
    def initialize(param_types, return_type)
      @param_types = param_types
      @return_type = return_type
    end

    attr_reader :param_types, :return_type
  end

  class DynamicLibrary
    def initialize(name, lib)
      @name = name
      @lib = lib
    end

    attr_reader :name
  end

  class AbstractMemory
    __bind_method__ :put_int8, :FFI_AbstractMemory_put_int8
    alias put_char put_int8
  end

  class Pointer < AbstractMemory
    __bind_method__ :address, :FFI_Pointer_address
    __bind_method__ :autorelease?, :FFI_Pointer_is_autorelease
    __bind_method__ :autorelease=, :FFI_Pointer_set_autorelease
    __bind_method__ :free, :FFI_Pointer_free
    __bind_method__ :initialize, :FFI_Pointer_initialize
    __bind_method__ :read_string, :FFI_Pointer_read_string
    __bind_method__ :to_obj, :FFI_Pointer_to_obj
    __bind_method__ :write_string, :FFI_Pointer_write_string

    def self.from_env
      __inline__ <<~END
        auto e = env->caller();
        return self.send(env, "new"_s, { Value::integer((uintptr_t)e) });
      END
    end

    def self.new_env
      __inline__ <<~END
        auto e = new Env { env->caller() };
        return self.send(env, "new"_s, { Value::integer((uintptr_t)e) });
      END
    end

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

class Object
  __bind_method__ :to_ptr, :Object_to_ptr
end
