#include "natalie.h"
#include "nat_kernel.h"
#include "nat_class.h"

NatObject *Kernel_puts(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    if (argc == 0) {
        printf("\n");
    } else {
        NatObject *str;
        for (size_t i=0; i<argc; i++) {
            str = nat_send(env, args[i], "to_s", 0, NULL);
            assert(str->type == NAT_VALUE_STRING);
            printf("%s\n", str->str);
        }
    }
    return env_get(env, "nil");
}

NatObject *Kernel_print(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    if (argc > 0) {
        NatObject *str;
        for (size_t i=0; i<argc; i++) {
            str = nat_send(env, args[i], "inspect", 0, NULL);
            assert(str->type == NAT_VALUE_STRING);
            printf("%s", str->str);
        }
    }
    return env_get(env, "nil");
}

NatObject *Kernel_p(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    if (argc == 0) {
        return env_get(env, "nil");
    } else if (argc == 1) {
        NatObject *arg = nat_send(env, args[0], "inspect", 0, NULL);
        Kernel_puts(env, self, 1, &arg, NULL);
        return arg;
    } else {
        NatObject *result = nat_array(env);
        for (size_t i=0; i<argc; i++) {
            nat_array_push(result, args[i]);
            args[i] = nat_send(env, args[i], "inspect", 0, NULL);
        }
        Kernel_puts(env, self, argc, args, kwargs);
        return result;
    }
}

NatObject *Kernel_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    NatObject *str = nat_string(env, "#<");
    assert(self->class);
    nat_string_append(str, Class_inspect(env, self->class, 0, NULL, NULL)->str);
    nat_string_append_char(str, ':');
    nat_string_append(str, nat_object_pointer_id(self));
    nat_string_append_char(str, '>');
    return str;
}

NatObject *Kernel_object_id(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_integer(env, self->id);
}

NatObject *Kernel_equal(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(argc == 1);
    NatObject *arg = args[0];
    if (self == arg) {
        return env_get(env, "true");
    } else {
        return env_get(env, "false");
    }
}

NatObject *Kernel_class(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return self->class ? self->class : env_get(env, "nil");
}
