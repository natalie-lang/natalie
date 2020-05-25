#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Kernel_puts(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *stdout = global_get(env, "$stdout");
    return IO_puts(env, stdout, argc, args, block);
}

Value *Kernel_print(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *stdout = global_get(env, "$stdout");
    return IO_print(env, stdout, argc, args, block);
}

Value *Kernel_p(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    if (argc == 0) {
        return NAT_NIL;
    } else if (argc == 1) {
        Value *arg = send(env, args[0], "inspect", 0, NULL, NULL);
        Kernel_puts(env, self, 1, &arg, NULL);
        return arg;
    } else {
        Value *result = array_new(env);
        for (ssize_t i = 0; i < argc; i++) {
            array_push(env, result, args[i]);
            args[i] = send(env, args[i], "inspect", 0, NULL, NULL);
        }
        Kernel_puts(env, self, argc, args, NULL);
        return result;
    }
}

Value *Kernel_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if ((NAT_TYPE(self) == ValueType::Class || NAT_TYPE(self) == ValueType::Module) && self->class_name) {
        return string(env, self->class_name);
    } else {
        Value *str = string(env, "#<");
        assert(NAT_OBJ_CLASS(self));
        string_append(env, str, Module_inspect(env, NAT_OBJ_CLASS(self), 0, NULL, NULL)->str);
        string_append_char(env, str, ':');
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        object_pointer_id(self, buf);
        string_append(env, str, buf);
        string_append_char(env, str, '>');
        return str;
    }
}

Value *Kernel_object_id(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return integer(env, object_id(env, self));
}

Value *Kernel_equal(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (self == arg) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Kernel_class(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return NAT_OBJ_CLASS(self) ? NAT_OBJ_CLASS(self) : NAT_NIL;
}

Value *Kernel_singleton_class(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return singleton_class(env, self);
}

Value *Kernel_instance_variables(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *ary = array_new(env);
    if (NAT_TYPE(self) == ValueType::Integer) {
        return ary;
    }
    struct hashmap_iter *iter;
    if (self->ivars.table) {
        for (iter = hashmap_iter(&self->ivars); iter; iter = hashmap_iter_next(&self->ivars, iter)) {
            char *name = (char *)hashmap_iter_get_key(iter);
            array_push(env, ary, symbol(env, name));
        }
    }
    return ary;
}

Value *Kernel_instance_variable_get(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    if (NAT_TYPE(self) == ValueType::Integer) {
        return NAT_NIL;
    }
    Value *name_obj = args[0];
    const char *name = NULL;
    if (NAT_TYPE(name_obj) == ValueType::String) {
        name = name_obj->str;
    } else if (NAT_TYPE(name_obj) == ValueType::Symbol) {
        name = name_obj->symbol;
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    return ivar_get(env, self, name);
}

Value *Kernel_instance_variable_set(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    NAT_ASSERT_NOT_FROZEN(self);
    Value *name_obj = args[0];
    const char *name = NULL;
    if (NAT_TYPE(name_obj) == ValueType::String) {
        name = name_obj->str;
    } else if (NAT_TYPE(name_obj) == ValueType::Symbol) {
        name = name_obj->symbol;
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    Value *val_obj = args[1];
    ivar_set(env, self, name, val_obj);
    return val_obj;
}

Value *Kernel_raise(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    Value *klass;
    Value *message;
    if (argc == 2) {
        klass = args[0];
        message = args[1];
    } else {
        Value *arg = args[0];
        if (NAT_TYPE(arg) == ValueType::Class) {
            klass = arg;
            message = string(env, arg->class_name);
        } else if (NAT_TYPE(arg) == ValueType::String) {
            klass = const_get(env, NAT_OBJECT, "RuntimeError", true);
            message = arg;
        } else if (is_a(env, arg, const_get(env, NAT_OBJECT, "Exception", true))) {
            raise_exception(env, arg);
            abort();
        } else {
            NAT_RAISE(env, "TypeError", "exception klass/object expected");
        }
    }
    raise(env, klass, message->str);
    abort();
}

Value *Kernel_respond_to(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *symbol = args[0];
    const char *name;
    if (NAT_TYPE(symbol) == ValueType::Symbol) {
        name = symbol->symbol;
    } else if (NAT_TYPE(symbol) == ValueType::String) {
        name = symbol->str;
    } else {
        return NAT_FALSE;
    }
    if (respond_to(env, self, name)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Kernel_dup(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return dup(env, self);
}

Value *Kernel_methods(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0); // for now
    Value *array = array_new(env);
    if (self->singleton_class) {
        methods(env, array, self->singleton_class);
    } else {
        methods(env, array, NAT_OBJ_CLASS(self));
    }
    return array;
}

Value *Kernel_exit(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    Value *status;
    if (argc == 1) {
        status = args[0];
        if (NAT_TYPE(status) != ValueType::Integer) {
            status = integer(env, 0);
        }
    } else {
        status = integer(env, 0);
    }
    Value *exception = exception_new(env, const_get(env, NAT_OBJECT, "SystemExit", true), "exit");
    ivar_set(env, exception, "@status", status);
    raise_exception(env, exception);
    return NAT_NIL;
}

Value *Kernel_at_exit(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *at_exit_handlers = global_get(env, "$NAT_at_exit_handlers");
    assert(at_exit_handlers);
    NAT_ASSERT_BLOCK();
    Value *proc = proc_new(env, block);
    array_push(env, at_exit_handlers, proc);
    return proc;
}

Value *Kernel_is_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *klass_or_module = args[0];
    if (NAT_TYPE(klass_or_module) != ValueType::Class && NAT_TYPE(klass_or_module) != ValueType::Module) {
        NAT_RAISE(env, "TypeError", "class or module required");
    }
    if (is_a(env, self, klass_or_module)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Kernel_hash(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    Value *inspected = send(env, self, "inspect", 0, NULL, NULL);
    ssize_t hash_value = hashmap_hash_string(inspected->str);
    ssize_t truncated_hash_value = RSHIFT(hash_value, 1); // shift right to fit in our int size (without need for BigNum)
    return integer(env, truncated_hash_value);
}

Value *Kernel_proc(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (block) {
        return proc_new(env, block);
    } else {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
}

Value *Kernel_lambda(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (block) {
        return lambda(env, block);
    } else {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
}

Value *Kernel_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    const char *name = find_current_method_name(env->caller);
    if (name) {
        return symbol(env, name);
    } else {
        return NAT_NIL;
    }
}

Value *Kernel_freeze(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    freeze_object(self);
    return self;
}

Value *Kernel_is_nil(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return NAT_FALSE;
}

Value *Kernel_sleep(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    if (argc == 0) {
        while (true) {
            sleep(1000);
        }
        abort(); // not reached
    } else {
        Value *length = args[0];
        NAT_ASSERT_TYPE(length, ValueType::Integer, "Integer"); // TODO: float supported also
        sleep(NAT_INT_VALUE(length));
        return length;
    }
}

Value *Kernel_define_singleton_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_BLOCK();
    Value *name_obj = args[0];
    if (NAT_TYPE(name_obj) == ValueType::Symbol) {
        // we're good!
    } else if (NAT_TYPE(name_obj) == ValueType::String) {
        name_obj = symbol(env, name_obj->str);
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    define_singleton_method_with_block(env, self, name_obj->symbol, block);
    return name_obj;
}

Value *Kernel_tap(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, NULL);
    return self;
}

Value *Kernel_Array(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *value = args[0];
    if (NAT_TYPE(value) == ValueType::Array) {
        return value;
    } else if (respond_to(env, value, "to_ary")) {
        return send(env, value, "to_ary", 0, NULL, NULL);
    } else if (value == NAT_NIL) {
        return array_new(env);
    } else {
        Value *ary = array_new(env);
        array_push(env, ary, value);
        return ary;
    }
}

Value *Kernel_send(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    Value *name_obj = args[0];
    const char *name;
    if (NAT_TYPE(name_obj) == ValueType::Symbol) {
        name = name_obj->symbol;
    } else if (NAT_TYPE(name_obj) == ValueType::String) {
        name = name_obj->str;
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    return send(env->caller, self, name, argc - 1, args + 1, block);
}

Value *Kernel_cur_dir(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (env->file == NULL) {
        NAT_RAISE(env, "RuntimeError", "could not get current directory");
    } else if (strcmp(env->file, "-e") == 0) {
        return string(env, ".");
    } else {
        Value *relative = string(env, env->file);
        Value *absolute = File_expand_path(env, const_get(env, NAT_OBJECT, "File", true), 1, &relative, NULL);
        ssize_t last_slash = 0;
        bool found = false;
        for (ssize_t i = 0; i < absolute->str_len; i++) {
            if (absolute->str[i] == '/') {
                found = true;
                last_slash = i;
            }
        }
        if (found) {
            absolute->str[last_slash] = 0;
            absolute->str_len = last_slash;
        }
        return absolute;
    }
}

}
