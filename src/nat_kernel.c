#include "natalie.h"
#include "nat_kernel.h"
#include "nat_module.h"

NatObject *Kernel_puts(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    if (argc == 0) {
        printf("\n");
    } else {
        NatObject *str;
        for (size_t i=0; i<argc; i++) {
            str = nat_send(env, args[i], "to_s", 0, NULL, NULL);
            assert(NAT_TYPE(str) == NAT_VALUE_STRING);
            printf("%s\n", str->str);
        }
    }
    return nil;
}

NatObject *Kernel_print(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    if (argc > 0) {
        NatObject *str;
        for (size_t i=0; i<argc; i++) {
            str = nat_send(env, args[i], "to_s", 0, NULL, NULL);
            assert(NAT_TYPE(str) == NAT_VALUE_STRING);
            printf("%s", str->str);
        }
    }
    return nil;
}

NatObject *Kernel_p(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    if (argc == 0) {
        return nil;
    } else if (argc == 1) {
        NatObject *arg = nat_send(env, args[0], "inspect", 0, NULL, NULL);
        Kernel_puts(env, self, 1, &arg, NULL, NULL);
        return arg;
    } else {
        NatObject *result = nat_array(env);
        for (size_t i=0; i<argc; i++) {
            nat_array_push(result, args[i]);
            args[i] = nat_send(env, args[i], "inspect", 0, NULL, NULL);
        }
        Kernel_puts(env, self, argc, args, kwargs, NULL);
        return result;
    }
}

NatObject *Kernel_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    if ((NAT_TYPE(self) == NAT_VALUE_CLASS || NAT_TYPE(self) == NAT_VALUE_MODULE) && self->class_name) {
        return nat_string(env, self->class_name);
    } else {
        NatObject *str = nat_string(env, "#<");
        assert(self->klass);
        nat_string_append(str, Module_inspect(env, self->klass, 0, NULL, NULL, NULL)->str);
        nat_string_append_char(str, ':');
        nat_string_append(str, nat_object_pointer_id(self));
        nat_string_append_char(str, '>');
        return str;
    }
}

NatObject *Kernel_object_id(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_integer(env, self->id);
}

NatObject *Kernel_equal(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (self == arg) {
        return true_obj;
    } else {
        return false_obj;
    }
}

NatObject *Kernel_class(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return self->klass ? self->klass : nil;
}

NatObject *Kernel_singleton_class(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_singleton_class(env, self);
}

NatObject *Kernel_instance_variable_get(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = args[0];
    assert(NAT_TYPE(name_obj) == NAT_VALUE_STRING || NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL);
    char *name = NAT_TYPE(name_obj) == NAT_VALUE_STRING ? name_obj->str : name_obj->symbol;
    return nat_ivar_get(env, self, name);
}

NatObject *Kernel_instance_variable_set(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(2);
    NatObject *name_obj = args[0];
    assert(NAT_TYPE(name_obj) == NAT_VALUE_STRING || NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL);
    char *name = NAT_TYPE(name_obj) == NAT_VALUE_STRING ? name_obj->str : name_obj->symbol;
    NatObject *val_obj = args[1];
    nat_ivar_set(env, self, name, val_obj);
    return val_obj;
}

NatObject *Kernel_raise(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1, 2);
    NatObject *klass;
    NatObject *message;
    if (argc == 2) {
        klass = args[0];
        message = args[1];
    } else if (argc == 1) {
        NatObject *arg = args[0];
        if (NAT_TYPE(arg) == NAT_VALUE_CLASS) {
            klass = arg;
            message = nat_string(env, arg->class_name);
        } else if (NAT_TYPE(arg) == NAT_VALUE_STRING) {
            klass = nat_const_get(env, Object, "RuntimeError");
            message = arg;
        } else if (nat_is_a(env, arg, nat_const_get(env, Object, "Exception"))) {
            nat_raise_exception(env, arg);
            abort();
        } else {
            NAT_RAISE(env, nat_const_get(env, Object, "TypeError"), "exception klass/object expected");
        }
    }
    NAT_RAISE(env, klass, message->str);
}

NatObject *Kernel_respond_to(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *symbol = args[0];
    char *name;
    if (NAT_TYPE(symbol) == NAT_VALUE_SYMBOL) {
        name = symbol->symbol;
    } else if (NAT_TYPE(symbol) == NAT_VALUE_STRING) {
        name = symbol->str;
    } else {
        return false_obj;
    }
    if (nat_respond_to(env, self, name)) {
        return true_obj;
    } else {
        return false_obj;
    }
}

NatObject *Kernel_dup(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_dup(env, self);
}

NatObject *Kernel_methods(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0); // for now
    NatObject *array = nat_array(env);
    if (self->singleton_class) {
        nat_methods(env, array, self->singleton_class);
    } else {
        nat_methods(env, array, self->klass);
    }
    return array;
}

NatObject *Kernel_exit(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0, 1);
    NatObject *status;
    if (argc == 1) {
        status = args[0];
        assert(NAT_TYPE(status) == NAT_VALUE_INTEGER);
    } else {
        status = nat_integer(env, 0);
    }
    NatObject *exception = nat_exception(env, nat_const_get(env, Object, "SystemExit"), "exit");
    nat_ivar_set(env, exception, "@status", status);
    nat_raise_exception(env, exception);
    return nil;
}

NatObject *Kernel_at_exit(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *at_exit_handlers = nat_global_get(env, "$NAT_at_exit_handlers");
    assert(at_exit_handlers);
    assert(block);
    NatObject *proc = nat_proc(env, block);
    nat_array_push(at_exit_handlers, proc);
    return proc;
}

NatObject *Kernel_is_a(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *klass_or_module = args[0];
    assert(NAT_TYPE(klass_or_module) == NAT_VALUE_CLASS || NAT_TYPE(klass_or_module) == NAT_VALUE_MODULE);
    if (nat_is_a(env, self, klass_or_module)) {
        return true_obj;
    } else {
        return false_obj;
    }
}

NatObject *Kernel_hash(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    NatObject *inspected = nat_send(env, self, "inspect", 0, NULL, NULL);
    int32_t hash_value = hashmap_hash_string(inspected->str);
    return nat_integer(env, hash_value);
}

NatObject *Kernel_proc(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(block);
    return nat_proc(env, block);
}

NatObject *Kernel_method(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    char *name = nat_find_current_method_name(env->caller);
    if (name) {
        return nat_string(env, name);
    } else {
        return nil;
    }
}
