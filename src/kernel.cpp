#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Kernel_puts(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *stdout = global_get(env, "$stdout");
    return IO_puts(env, stdout, argc, args, block);
}

NatObject *Kernel_print(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *stdout = global_get(env, "$stdout");
    return IO_print(env, stdout, argc, args, block);
}

NatObject *Kernel_p(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    if (argc == 0) {
        return NAT_NIL;
    } else if (argc == 1) {
        NatObject *arg = send(env, args[0], "inspect", 0, NULL, NULL);
        Kernel_puts(env, self, 1, &arg, NULL);
        return arg;
    } else {
        NatObject *result = array_new(env);
        for (ssize_t i = 0; i < argc; i++) {
            array_push(env, result, args[i]);
            args[i] = send(env, args[i], "inspect", 0, NULL, NULL);
        }
        Kernel_puts(env, self, argc, args, NULL);
        return result;
    }
}

NatObject *Kernel_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if ((NAT_TYPE(self) == NAT_VALUE_CLASS || NAT_TYPE(self) == NAT_VALUE_MODULE) && self->class_name) {
        return string(env, self->class_name);
    } else {
        NatObject *str = string(env, "#<");
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

NatObject *Kernel_object_id(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return integer(env, object_id(env, self));
}

NatObject *Kernel_equal(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (self == arg) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Kernel_class(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return NAT_OBJ_CLASS(self) ? NAT_OBJ_CLASS(self) : NAT_NIL;
}

NatObject *Kernel_singleton_class(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return singleton_class(env, self);
}

NatObject *Kernel_instance_variables(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *ary = array_new(env);
    if (NAT_TYPE(self) == NAT_VALUE_INTEGER) {
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

NatObject *Kernel_instance_variable_get(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    if (NAT_TYPE(self) == NAT_VALUE_INTEGER) {
        return NAT_NIL;
    }
    NatObject *name_obj = args[0];
    const char *name = NULL;
    if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
        name = name_obj->str;
    } else if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
        name = name_obj->symbol;
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    return ivar_get(env, self, name);
}

NatObject *Kernel_instance_variable_set(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *name_obj = args[0];
    const char *name = NULL;
    if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
        name = name_obj->str;
    } else if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
        name = name_obj->symbol;
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    NatObject *val_obj = args[1];
    ivar_set(env, self, name, val_obj);
    return val_obj;
}

NatObject *Kernel_raise(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    NatObject *klass;
    NatObject *message;
    if (argc == 2) {
        klass = args[0];
        message = args[1];
    } else {
        NatObject *arg = args[0];
        if (NAT_TYPE(arg) == NAT_VALUE_CLASS) {
            klass = arg;
            message = string(env, arg->class_name);
        } else if (NAT_TYPE(arg) == NAT_VALUE_STRING) {
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

NatObject *Kernel_respond_to(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *symbol = args[0];
    const char *name;
    if (NAT_TYPE(symbol) == NAT_VALUE_SYMBOL) {
        name = symbol->symbol;
    } else if (NAT_TYPE(symbol) == NAT_VALUE_STRING) {
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

NatObject *Kernel_dup(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return dup(env, self);
}

NatObject *Kernel_methods(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0); // for now
    NatObject *array = array_new(env);
    if (self->singleton_class) {
        methods(env, array, self->singleton_class);
    } else {
        methods(env, array, NAT_OBJ_CLASS(self));
    }
    return array;
}

NatObject *Kernel_exit(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    NatObject *status;
    if (argc == 1) {
        status = args[0];
        if (NAT_TYPE(status) != NAT_VALUE_INTEGER) {
            status = integer(env, 0);
        }
    } else {
        status = integer(env, 0);
    }
    NatObject *exception = exception_new(env, const_get(env, NAT_OBJECT, "SystemExit", true), "exit");
    ivar_set(env, exception, "@status", status);
    raise_exception(env, exception);
    return NAT_NIL;
}

NatObject *Kernel_at_exit(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *at_exit_handlers = global_get(env, "$NAT_at_exit_handlers");
    assert(at_exit_handlers);
    NAT_ASSERT_BLOCK();
    NatObject *proc = proc_new(env, block);
    array_push(env, at_exit_handlers, proc);
    return proc;
}

NatObject *Kernel_is_a(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *klass_or_module = args[0];
    if (NAT_TYPE(klass_or_module) != NAT_VALUE_CLASS && NAT_TYPE(klass_or_module) != NAT_VALUE_MODULE) {
        NAT_RAISE(env, "TypeError", "class or module required");
    }
    if (is_a(env, self, klass_or_module)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Kernel_hash(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    NatObject *inspected = send(env, self, "inspect", 0, NULL, NULL);
    ssize_t hash_value = hashmap_hash_string(inspected->str);
    ssize_t truncated_hash_value = RSHIFT(hash_value, 1); // shift right to fit in our int size (without need for BigNum)
    return integer(env, truncated_hash_value);
}

NatObject *Kernel_proc(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (block) {
        return proc_new(env, block);
    } else {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
}

NatObject *Kernel_lambda(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (block) {
        return lambda(env, block);
    } else {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
}

NatObject *Kernel_method(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    const char *name = find_current_method_name(env->caller);
    if (name) {
        return symbol(env, name);
    } else {
        return NAT_NIL;
    }
}

NatObject *Kernel_freeze(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    freeze_object(self);
    return self;
}

NatObject *Kernel_is_nil(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return NAT_FALSE;
}

NatObject *Kernel_sleep(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    if (argc == 0) {
        while (true) {
            sleep(1000);
        }
        abort(); // not reached
    } else {
        NatObject *length = args[0];
        NAT_ASSERT_TYPE(length, NAT_VALUE_INTEGER, "Integer"); // TODO: float supported also
        sleep(NAT_INT_VALUE(length));
        return length;
    }
}

NatObject *Kernel_define_singleton_method(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_BLOCK();
    NatObject *name_obj = args[0];
    if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
        // we're good!
    } else if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
        name_obj = symbol(env, name_obj->str);
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    define_singleton_method_with_block(env, self, name_obj->symbol, block);
    return name_obj;
}

NatObject *Kernel_tap(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, NULL);
    return self;
}

NatObject *Kernel_Array(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *value = args[0];
    if (NAT_TYPE(value) == NAT_VALUE_ARRAY) {
        return value;
    } else if (respond_to(env, value, "to_ary")) {
        return send(env, value, "to_ary", 0, NULL, NULL);
    } else if (value == NAT_NIL) {
        return array_new(env);
    } else {
        NatObject *ary = array_new(env);
        array_push(env, ary, value);
        return ary;
    }
}

NatObject *Kernel_send(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    NatObject *name_obj = args[0];
    const char *name;
    if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
        name = name_obj->symbol;
    } else if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
        name = name_obj->str;
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    return send(env->caller, self, name, argc - 1, args + 1, block);
}

NatObject *Kernel_cur_dir(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (env->file == NULL) {
        NAT_RAISE(env, "RuntimeError", "could not get current directory");
    } else if (strcmp(env->file, "-e") == 0) {
        return string(env, ".");
    } else {
        NatObject *relative = string(env, env->file);
        NatObject *absolute = File_expand_path(env, const_get(env, NAT_OBJECT, "File", true), 1, &relative, NULL);
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
