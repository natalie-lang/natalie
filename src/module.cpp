#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Module_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return module(env, NULL);
}

NatObject *Module_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    if (self->class_name) {
        if (self->owner && self->owner != NAT_OBJECT) {
            return sprintf(env, "%S::%s", Module_inspect(env, self->owner, 0, NULL, NULL), self->class_name);
        } else {
            return string(env, self->class_name);
        }
    } else if (NAT_TYPE(self) == NAT_VALUE_CLASS) {
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        object_pointer_id(self, buf);
        return sprintf(env, "#<Class:%s>", buf);
    } else {
        return Kernel_inspect(env, self, argc, args, block);
    }
}

NatObject *Module_eqeqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (is_a(env, arg, self)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Module_name(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return self->class_name ? string(env, self->class_name) : NAT_NIL;
}

NatObject *Module_ancestors(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return class_ancestors(env, self);
}

NatObject *Module_attr_reader(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        NatObject *name_obj = args[i];
        if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
            name_obj = string(env, name_obj->symbol);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        Env block_env;
        build_detached_block_env(&block_env, env);
        var_set(&block_env, "name", 0, true, name_obj);
        Block *attr_block = block_new(&block_env, self, Module_attr_reader_block_fn);
        define_method_with_block(env, self, name_obj->str, attr_block);
    }
    return NAT_NIL;
}

NatObject *Module_attr_reader_block_fn(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *name_obj = var_get(env->outer, "name", 0);
    assert(name_obj);
    NatObject *ivar_name = sprintf(env, "@%S", name_obj);
    return ivar_get(env, self, ivar_name->str);
}

NatObject *Module_attr_writer(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        NatObject *name_obj = args[i];
        if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
            name_obj = string(env, name_obj->symbol);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        NatObject *method_name = string(env, name_obj->str);
        string_append_char(env, method_name, '=');
        Env block_env;
        build_detached_block_env(&block_env, env);
        var_set(&block_env, "name", 0, true, name_obj);
        Block *attr_block = block_new(&block_env, self, Module_attr_writer_block_fn);
        define_method_with_block(env, self, method_name->str, attr_block);
    }
    return NAT_NIL;
}

NatObject *Module_attr_writer_block_fn(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *val = args[0];
    NatObject *name_obj = var_get(env->outer, "name", 0);
    assert(name_obj);
    NatObject *ivar_name = sprintf(env, "@%S", name_obj);
    ivar_set(env, self, ivar_name->str, val);
    return val;
}

NatObject *Module_attr_accessor(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    Module_attr_reader(env, self, argc, args, block);
    Module_attr_writer(env, self, argc, args, block);
    return NAT_NIL;
}

NatObject *Module_include(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        class_include(env, self, args[i]);
    }
    return self;
}

NatObject *Module_prepend(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (int i = argc - 1; i >= 0; i--) {
        class_prepend(env, self, args[i]);
    }
    return self;
}

NatObject *Module_included_modules(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    NatObject *modules = array_new(env);
    for (ssize_t i = 0; i < self->included_modules_count; i++) {
        array_push(env, modules, self->included_modules[i]);
    }
    return modules;
}

NatObject *Module_define_method(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = args[0];
    if (NAT_TYPE(name_obj) != NAT_VALUE_STRING && NAT_TYPE(name_obj) != NAT_VALUE_SYMBOL) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    if (!block) {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
    char *name = NAT_TYPE(name_obj) == NAT_VALUE_STRING ? name_obj->str : name_obj->symbol;
    define_method_with_block(env, self, name, block);
    return symbol(env, name);
}

NatObject *Module_class_eval(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    if (argc > 0 || !block) {
        NAT_RAISE(env, "ArgumentError", "Natalie only supports class_eval with a block");
    }
    Env e;
    build_block_env(&e, &block->env, env);
    return block->fn(&e, self, 0, NULL, NULL);
}

NatObject *Module_private(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    printf("TODO: class private\n");
    return NAT_NIL;
}

NatObject *Module_protected(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    printf("TODO: class protected\n");
    return NAT_NIL;
}

NatObject *Module_const_defined(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = args[0];
    if (NAT_TYPE(name_obj) == NAT_VALUE_STRING) {
        // we're good!
    } else if (NAT_TYPE(name_obj) == NAT_VALUE_SYMBOL) {
        name_obj = string(env, name_obj->symbol);
    } else {
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s to String", send(env, name_obj->klass, "inspect", 0, NULL, NULL));
    }
    if (const_get_or_null(env, self, name_obj->str, false, false)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Module_alias_method(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_MODULE || NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(2);
    NatObject *new_name = args[0];
    if (NAT_TYPE(new_name) == NAT_VALUE_STRING) {
        // we're good!
    } else if (NAT_TYPE(new_name) == NAT_VALUE_SYMBOL) {
        new_name = string(env, new_name->symbol);
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", send(env, new_name, "inspect", 0, NULL, NULL));
    }
    NatObject *old_name = args[1];
    if (NAT_TYPE(old_name) == NAT_VALUE_STRING) {
        // we're good!
    } else if (NAT_TYPE(old_name) == NAT_VALUE_SYMBOL) {
        old_name = string(env, old_name->symbol);
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", send(env, old_name, "inspect", 0, NULL, NULL));
    }
    alias(env, self, new_name->str, old_name->str);
    return self;
}

}
