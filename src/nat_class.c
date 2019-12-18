#include "natalie.h"
#include "nat_class.h"

NatObject *Class_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    return nat_subclass(env, self, NULL);
}

NatObject *Class_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
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
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i=0; i<argc; i++) {
        nat_class_include(self, args[i]);
    }
    return self;
}

NatObject *Class_included_modules(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    NatObject *modules = nat_array(env);
    for (size_t i=0; i<self->included_modules_count; i++) {
        nat_array_push(modules, self->included_modules[i]);
    }
    return modules;
}

NatObject *Class_ancestors(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
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
    NAT_ASSERT_ARGC(0);
    return self->superclass ? self->superclass : env_get(env, "nil");
}

NatObject *Class_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
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

NatObject *Class_define_method(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = args[0];
    assert(name_obj->type == NAT_VALUE_STRING || name_obj->type == NAT_VALUE_SYMBOL);
    assert(block);
    char *name = name_obj->type == NAT_VALUE_STRING ? name_obj->str : name_obj->symbol;
    nat_define_method_with_block(self, name, block);
    return env_get(env, "nil"); // FIXME: this should return a Proc I think
}

NatObject *Class_attr_reader(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i=0; i<argc; i++) {
        NatObject *name_obj = args[i];
        assert(name_obj->type == NAT_VALUE_STRING || name_obj->type == NAT_VALUE_SYMBOL);
        if (name_obj->type == NAT_VALUE_SYMBOL) name_obj = nat_string(env, name_obj->symbol);
        NatEnv *block_env = build_env(env);
        block_env->block = TRUE;
        env_set(block_env, "name", name_obj);
        NatBlock *block = nat_block(block_env, self, Class_attr_reader_block_fn);
        nat_define_method_with_block(self, name_obj->str, block);
    }
    return env_get(env, "nil");
}

NatObject *Class_attr_reader_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *name_obj = env_get(env, "name");
    assert(name_obj);
    NatObject *ivar_name = nat_string(env, "@");
    nat_string_append(ivar_name, name_obj->str);
    return ivar_get(env, self, ivar_name->str);
}

NatObject *Class_attr_writer(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i=0; i<argc; i++) {
        NatObject *name_obj = args[i];
        assert(name_obj->type == NAT_VALUE_STRING || name_obj->type == NAT_VALUE_SYMBOL);
        if (name_obj->type == NAT_VALUE_SYMBOL) name_obj = nat_string(env, name_obj->symbol);
        NatObject *method_name = nat_string(env, name_obj->str);
        nat_string_append_char(method_name, '=');
        NatEnv *block_env = build_env(env);
        block_env->block = TRUE;
        env_set(block_env, "name", name_obj);
        NatBlock *block = nat_block(block_env, self, Class_attr_writer_block_fn);
        nat_define_method_with_block(self, method_name->str, block);
    }
    return env_get(env, "nil");
}

NatObject *Class_attr_writer_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *val = args[0];
    NatObject *name_obj = env_get(env, "name");
    assert(name_obj);
    NatObject *ivar_name = nat_string(env, "@");
    nat_string_append(ivar_name, name_obj->str);
    ivar_set(env, self, ivar_name->str, val);
    return val;
}

NatObject *Class_attr_accessor(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    Class_attr_reader(env, self, argc, args, kwargs, block);
    Class_attr_writer(env, self, argc, args, kwargs, block);
    return env_get(env, "nil");
}
