require 'natalie/inline'

__inline__ '#include <dlfcn.h>'
__ld_flags__ '-ldl'

class Fiddle
  class DLError
  end

  TYPE_VOID = :void
  TYPE_VOIDP = :voidp

  class << self
    __define_method__ :dlopen, [:path], <<-END
      path->assert_type(env, Object::Type::String, "String");
      auto handle = dlopen(path->as_string()->c_str(), RTLD_LAZY);
      if (!handle) {
          auto dl_error = self->const_find(env, "DLError"_s, Object::ConstLookupSearchMode::NotStrict)->as_class();
          env->raise(dl_error, "{}", dlerror());
      }
      auto handle_class = self->const_find(env, "Handle"_s, Object::ConstLookupSearchMode::NotStrict)->as_class();
      auto handle_obj = new Object { Object::Type::Object, handle_class };
      auto handle_ptr = new VoidPObject { handle };
      handle_obj->ivar_set(env, "@ptr"_s, handle_ptr);
      return handle_obj;
    END
  end

  class Handle
    __define_method__ :[], [:name], <<-END
      auto handle = self->ivar_get(env, "@ptr"_s)->as_void_p()->void_ptr();
      name->assert_type(env, Object::Type::String, "String");
      auto symbol = dlsym(handle, name->as_string()->c_str());
      return new IntegerObject { (long long)symbol };
    END
  end

  class Pointer
  end

  class Function
    def initialize(symbol, arg_types, return_type)
      @symbol = symbol
      @arg_types = arg_types
      @return_type = return_type
    end

    def call(*args)
      if args.size != @arg_types.size
        raise ArgumentError, "wrong number of arguments (given #{args.size}, expected #{@arg_types.size})"
      end
      internal_method_name = "#{@return_type}_"
      if @arg_types.empty?
        internal_method_name += 'no_args'
      else
        internal_method_name += 'args_'
        internal_method_name += @arg_types.join('_')
      end
      if defined?(internal_method_name)
        send(internal_method_name, *args)
      else
        raise NoMethodError, "Fiddle: no method with name: #{internal_method_name}"
      end
    end

    private

    # NOTE: currently only supporting the few function signatures that we need for Natalie's REPL
    # In the future, Natalie will have macros that will allow this to be generated at compile time.

    __define_method__ :void_no_args, [], <<-END
      auto symbol = self->ivar_get(env, "@symbol"_s)->as_integer()->to_nat_int_t();
      auto fn = (void* (*)())symbol;
      fn();
      return NilObject::the();
    END

    __define_method__ :voidp_no_args, [], <<-END
      auto symbol = self->ivar_get(env, "@symbol"_s)->as_integer()->to_nat_int_t();
      auto fn = (void* (*)())symbol;
      auto result = fn();
      auto pointer_class = self->const_find(env, "Pointer"_s, Object::ConstLookupSearchMode::NotStrict)->as_class();
      auto pointer_obj = new Object { Object::Type::Object, pointer_class };
      auto pointer_ptr = new VoidPObject { result };
      pointer_obj->ivar_set(env, "@ptr"_s, pointer_ptr);
      return pointer_obj;
    END

    __define_method__ :voidp_args_voidp, [:p1], <<-END
      auto symbol = self->ivar_get(env, "@symbol"_s)->as_integer()->to_nat_int_t();
      auto fn = (void* (*)(void*))symbol;
      void *p1_ptr;
      auto pointer_class = self->const_find(env, "Pointer"_s, Object::ConstLookupSearchMode::NotStrict)->as_class();
      if (p1->is_a(env, pointer_class))
          p1_ptr = p1->ivar_get(env, "@ptr"_s)->as_void_p()->void_ptr();
      else if (p1->is_void_p())
          p1_ptr = p1->as_void_p()->void_ptr();
      else
          p1_ptr = (void*)(p1.object());
      auto result = fn(p1_ptr);
      auto pointer_obj = new Object { Object::Type::Object, pointer_class };
      auto pointer_ptr = new VoidPObject { result };
      pointer_obj->ivar_set(env, "@ptr"_s, pointer_ptr);
      return pointer_obj;
    END
  end
end
