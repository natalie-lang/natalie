#include "natalie.h"
#include "nat_module.h"
#include "nat_kernel.h"

NatObject *Module_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_module(env, NULL);
}

NatObject *Module_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    if (self->class_name) {
        return nat_string(env, self->class_name);
    } else if (NAT_TYPE(self) == NAT_VALUE_CLASS) {
        return nat_sprintf(env, "#<Class:%s>", nat_object_pointer_id(self));
    } else {
        return Kernel_inspect(env, self, argc, args, kwargs, block);
    }
}

NatObject *Module_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (nat_is_a(env, arg, self)) {
        return true_obj;
    } else {
        return false_obj;
    }
}

NatObject *Module_name(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return self->class_name ? nat_string(env, self->class_name) : nil;
}

NatObject *Module_ancestors(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return nat_class_ancestors(env, self);
}

NatObject *Module_attr_reader(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i=0; i<argc; i++) {
        NatObject *name_obj = args[i];
        assert(NAT_TYPE(name_obj) == NAT_VALUE_STRING || NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL);
        if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) name_obj = nat_string(env, name_obj->symbol);
        NatEnv *block_env = nat_build_env(env);
        block_env->block = TRUE;
        nat_var_set2(block_env, "name", 0, name_obj);
        NatBlock *block = nat_block(block_env, self, Module_attr_reader_block_fn);
        nat_define_method_with_block(self, name_obj->str, block);
    }
    return nil;
}

NatObject *Module_attr_reader_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *name_obj = nat_var_get2(env->outer, "name", 0);
    assert(name_obj);
    NatObject *ivar_name = nat_string(env, "@");
    nat_string_append(ivar_name, name_obj->str);
    return nat_ivar_get(env, self, ivar_name->str);
}

NatObject *Module_attr_writer(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i=0; i<argc; i++) {
        NatObject *name_obj = args[i];
        assert(NAT_TYPE(name_obj) == NAT_VALUE_STRING || NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL);
        if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) name_obj = nat_string(env, name_obj->symbol);
        NatObject *method_name = nat_string(env, name_obj->str);
        nat_string_append_char(method_name, '=');
        NatEnv *block_env = nat_build_env(env);
        block_env->block = TRUE;
        nat_var_set2(block_env, "name", 0, name_obj);
        NatBlock *block = nat_block(block_env, self, Module_attr_writer_block_fn);
        nat_define_method_with_block(self, method_name->str, block);
    }
    return nil;
}

NatObject *Module_attr_writer_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *val = args[0];
    NatObject *name_obj = nat_var_get2(env->outer, "name", 0);
    assert(name_obj);
    NatObject *ivar_name = nat_string(env, "@");
    nat_string_append(ivar_name, name_obj->str);
    nat_ivar_set(env, self, ivar_name->str, val);
    return val;
}

NatObject *Module_attr_accessor(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    Module_attr_reader(env, self, argc, args, kwargs, block);
    Module_attr_writer(env, self, argc, args, kwargs, block);
    return nil;
}

NatObject *Module_include(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i=0; i<argc; i++) {
        nat_class_include(self, args[i]);
    }
    return self;
}

NatObject *Module_included_modules(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    NatObject *modules = nat_array(env);
    for (size_t i=0; i<self->included_modules_count; i++) {
        nat_array_push(modules, self->included_modules[i]);
    }
    return modules;
}

NatObject *Module_define_method(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = args[0];
    assert(NAT_TYPE(name_obj) == NAT_VALUE_STRING || NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL);
    assert(block);
    char *name = NAT_TYPE(name_obj) == NAT_VALUE_STRING ? name_obj->str : name_obj->symbol;
    nat_define_method_with_block(self, name, block);
    return nat_symbol(env, name);
}

NatObject *Module_class_eval(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    assert(block);
    NatEnv *e = nat_build_block_env(block->env, env);
    return block->fn(e, self, 0, NULL, NULL, NULL);
}
