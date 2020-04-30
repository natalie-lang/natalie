#include "builtin.h"
#include "natalie.h"

NatObject *Module_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_module(env, NULL);
}

NatObject *Module_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    if (self->class_name) {
        if (self->owner && self->owner != NAT_OBJECT) {
            return nat_sprintf(env, "%S::%s", Module_inspect(env, self->owner, 0, NULL, NULL), self->class_name);
        } else {
            return nat_string(env, self->class_name);
        }
    } else if (NAT_TYPE(self) == NAT_VALUE_CLASS) {
        return nat_sprintf(env, "#<Class:%s>", nat_object_pointer_id(env, self));
    } else {
        return Kernel_inspect(env, self, argc, args, block);
    }
}

NatObject *Module_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (nat_is_a(env, arg, self)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Module_name(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return self->class_name ? nat_string(env, self->class_name) : NAT_NIL;
}

NatObject *Module_ancestors(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return nat_class_ancestors(env, self);
}

NatObject *Module_attr_reader(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i = 0; i < argc; i++) {
        NatObject *name_obj = args[i];
        if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
            name_obj = nat_string(env, name_obj->symbol);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", nat_send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        NatEnv *block_env = nat_build_detached_block_env(env);
        nat_var_set(block_env, "name", 0, true, name_obj);
        NatBlock *attr_block = nat_block(block_env, self, Module_attr_reader_block_fn);
        nat_define_method_with_block(env, self, name_obj->str, attr_block);
    }
    return NAT_NIL;
}

NatObject *Module_attr_reader_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NatObject *name_obj = nat_var_get(env->outer, "name", 0);
    assert(name_obj);
    NatObject *ivar_name = nat_sprintf(env, "@%S", name_obj);
    return nat_ivar_get(env, self, ivar_name->str);
}

NatObject *Module_attr_writer(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i = 0; i < argc; i++) {
        NatObject *name_obj = args[i];
        if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
            name_obj = nat_string(env, name_obj->symbol);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", nat_send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        NatObject *method_name = nat_string(env, name_obj->str);
        nat_string_append_char(env, method_name, '=');
        NatEnv *block_env = nat_build_detached_block_env(env);
        nat_var_set(block_env, "name", 0, true, name_obj);
        NatBlock *attr_block = nat_block(block_env, self, Module_attr_writer_block_fn);
        nat_define_method_with_block(env, self, method_name->str, attr_block);
    }
    return NAT_NIL;
}

NatObject *Module_attr_writer_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NatObject *val = args[0];
    NatObject *name_obj = nat_var_get(env->outer, "name", 0);
    assert(name_obj);
    NatObject *ivar_name = nat_sprintf(env, "@%S", name_obj);
    nat_ivar_set(env, self, ivar_name->str, val);
    return val;
}

NatObject *Module_attr_accessor(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    Module_attr_reader(env, self, argc, args, block);
    Module_attr_writer(env, self, argc, args, block);
    return NAT_NIL;
}

NatObject *Module_include(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (size_t i = 0; i < argc; i++) {
        nat_class_include(env, self, args[i]);
    }
    return self;
}

NatObject *Module_prepend(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (int i = argc - 1; i >= 0; i--) {
        nat_class_prepend(env, self, args[i]);
    }
    return self;
}

NatObject *Module_included_modules(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    NatObject *modules = nat_array(env);
    for (size_t i = 0; i < self->included_modules_count; i++) {
        nat_array_push(env, modules, self->included_modules[i]);
    }
    return modules;
}

NatObject *Module_define_method(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = args[0];
    if (NAT_TYPE(name_obj) != NAT_VALUE_STRING && NAT_TYPE(name_obj) != NAT_VALUE_SYMBOL) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", nat_send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    if (!block) {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
    char *name = NAT_TYPE(name_obj) == NAT_VALUE_STRING ? name_obj->str : name_obj->symbol;
    nat_define_method_with_block(env, self, name, block);
    return nat_symbol(env, name);
}

NatObject *Module_class_eval(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    if (argc > 0 || !block) {
        NAT_RAISE(env, "ArgumentError", "Natalie only supports class_eval with a block");
    }
    NatEnv *e = nat_build_block_env(block->env, env);
    return block->fn(e, self, 0, NULL, NULL);
}

NatObject *Module_private(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    printf("TODO: class private\n");
    return NAT_NIL;
}

NatObject *Module_protected(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    printf("TODO: class protected\n");
    return NAT_NIL;
}

NatObject *Module_const_defined(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = args[0];
    if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
        // we're good!
    } else if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
        name_obj = nat_string(env, name_obj->symbol);
    } else {
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s to String", nat_send(env, name_obj->klass, "inspect", 0, NULL, NULL));
    }
    if (nat_const_get_or_null(env, self, name_obj->str, false, false)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Module_alias_method(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(2);
    NatObject *new_name = args[0];
    if (NAT_TYPE(new_name) == NAT_VALUE_STRING) {
        // we're good!
    } else if (NAT_TYPE(new_name) == NAT_VALUE_SYMBOL) {
        new_name = nat_string(env, new_name->symbol);
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", nat_send(env, new_name, "inspect", 0, NULL, NULL));
    }
    NatObject *old_name = args[1];
    if (NAT_TYPE(old_name) == NAT_VALUE_STRING) {
        // we're good!
    } else if (NAT_TYPE(old_name) == NAT_VALUE_SYMBOL) {
        old_name = nat_string(env, old_name->symbol);
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", nat_send(env, old_name, "inspect", 0, NULL, NULL));
    }
    nat_alias(env, self, new_name->str, old_name->str);
    return self;
}
