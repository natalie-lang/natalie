#include "natalie.h"
#include "nat_class.h"

NatObject *Class_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    return nat_subclass(env, self, NULL);
}

NatObject *Class_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    if (self->class_name) {
        return nat_string(env, self->class_name);
    } else {
        NatObject *str = nat_string(env, "#<");
        assert(self->class);
        nat_string_append(str, Class_inspect(env, self->superclass, 0, NULL, NULL, NULL)->str);
        nat_string_append_char(str, ':');
        nat_string_append(str, nat_object_pointer_id(self));
        nat_string_append_char(str, '>');
        return str;
    }
}

NatObject *Class_include(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    assert(argc >= 1);
    for (size_t i=0; i<argc; i++) {
        nat_class_include(self, args[i]);
    }
    return self;
}

NatObject *Class_included_modules(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    NatObject *modules = nat_array(env);
    for (size_t i=0; i<self->included_modules_count; i++) {
        nat_array_push(modules, self->included_modules[i]);
    }
    return modules;
}

NatObject *Class_ancestors(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    assert(argc == 0);
    NatObject *ancestors = nat_array(env);
    NatObject *class = self;
    while (1) {
        nat_array_push(ancestors, class);
        for (size_t i=0; i<class->included_modules_count; i++) {
            nat_array_push(ancestors, class->included_modules[i]);
        }
        if (nat_is_top_class(class)) break;
        class = class->superclass;
    }
    return ancestors;
}

NatObject *Class_superclass(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    return self->superclass ? self->superclass : env_get(env, "nil");
}

NatObject *Class_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    assert(argc == 1);
    NatObject *arg = args[0];
    if (self == arg) {
        return env_get(env, "true");
    } else {
        NatObject *ancestors = Class_ancestors(env, arg->class, 0, NULL, NULL, NULL);
        for (size_t i=0; i<ancestors->ary_len; i++) {
            if (self == ancestors->ary[i]) {
                return env_get(env, "true");
            }
        }
        return env_get(env, "false");
    }
}
