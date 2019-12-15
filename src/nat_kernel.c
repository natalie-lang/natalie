#include "natalie.h"
#include "nat_kernel.h"
#include "nat_class.h"

NatObject *Kernel_puts(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    if (argc == 0) {
        printf("\n");
    } else {
        NatObject *str;
        for (size_t i=0; i<argc; i++) {
            str = nat_send(env, args[i], "to_s", 0, NULL, NULL);
            assert(str->type == NAT_VALUE_STRING);
            printf("%s\n", str->str);
        }
    }
    return env_get(env, "nil");
}

NatObject *Kernel_print(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    if (argc > 0) {
        NatObject *str;
        for (size_t i=0; i<argc; i++) {
            str = nat_send(env, args[i], "inspect", 0, NULL, NULL);
            assert(str->type == NAT_VALUE_STRING);
            printf("%s", str->str);
        }
    }
    return env_get(env, "nil");
}

NatObject *Kernel_p(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    if (argc == 0) {
        return env_get(env, "nil");
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
    NatObject *str = nat_string(env, "#<");
    assert(self->class);
    nat_string_append(str, Class_inspect(env, self->class, 0, NULL, NULL, NULL)->str);
    nat_string_append_char(str, ':');
    nat_string_append(str, nat_object_pointer_id(self));
    nat_string_append_char(str, '>');
    return str;
}

NatObject *Kernel_object_id(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_integer(env, self->id);
}

NatObject *Kernel_equal(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (self == arg) {
        return env_get(env, "true");
    } else {
        return env_get(env, "false");
    }
}

NatObject *Kernel_class(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return self->class ? self->class : env_get(env, "nil");
}

NatObject *Kernel_instance_variable_get(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = args[0];
    assert(name_obj->type == NAT_VALUE_STRING || name_obj->type == NAT_VALUE_SYMBOL);
    char *name = name_obj->type == NAT_VALUE_STRING ? name_obj->str : name_obj->symbol;
    return ivar_get(env, self, name);
}

NatObject *Kernel_instance_variable_set(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(2);
    NatObject *name_obj = args[0];
    assert(name_obj->type == NAT_VALUE_STRING || name_obj->type == NAT_VALUE_SYMBOL);
    char *name = name_obj->type == NAT_VALUE_STRING ? name_obj->str : name_obj->symbol;
    NatObject *val_obj = args[1];
    ivar_set(env, self, name, val_obj);
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
        if (arg->type == NAT_VALUE_CLASS) {
            klass = arg;
            message = nat_string(env, arg->class_name);
        } else if (arg->type == NAT_VALUE_STRING) {
            klass = env_get(env, "RuntimeError");
            message = arg;
        } else if (Class_eqeqeq(env, env_get(env, "Exception"), 1, &arg, NULL, NULL)) {
            nat_raise_exception(env, arg);
            abort();
        } else {
            NAT_RAISE(env, env_get(env, "TypeError"), "exception class/object expected");
        }
    }
    NAT_RAISE(env, klass, message->str);
}
