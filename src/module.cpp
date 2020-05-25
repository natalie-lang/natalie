#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Module_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return module(env, NULL);
}

Value *Module_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC(0);
    if (self->class_name) {
        if (self->owner && self->owner != NAT_OBJECT) {
            return sprintf(env, "%S::%s", Module_inspect(env, self->owner, 0, NULL, NULL), self->class_name);
        } else {
            return string(env, self->class_name);
        }
    } else if (NAT_TYPE(self) == ValueType::Class) {
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        object_pointer_id(self, buf);
        return sprintf(env, "#<Class:%s>", buf);
    } else {
        return Kernel_inspect(env, self, argc, args, block);
    }
}

Value *Module_eqeqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (is_a(env, arg, self)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Module_name(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC(0);
    return self->class_name ? string(env, self->class_name) : NAT_NIL;
}

Value *Module_ancestors(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC(0);
    return class_ancestors(env, self);
}

Value *Module_attr_reader(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        Value *name_obj = args[i];
        if (NAT_TYPE(name_obj) == ValueType::String) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == ValueType::Symbol) {
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

Value *Module_attr_reader_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *name_obj = var_get(env->outer, "name", 0);
    assert(name_obj);
    Value *ivar_name = sprintf(env, "@%S", name_obj);
    return ivar_get(env, self, ivar_name->str);
}

Value *Module_attr_writer(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        Value *name_obj = args[i];
        if (NAT_TYPE(name_obj) == ValueType::String) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == ValueType::Symbol) {
            name_obj = string(env, name_obj->symbol);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        Value *method_name = string(env, name_obj->str);
        string_append_char(env, method_name, '=');
        Env block_env;
        build_detached_block_env(&block_env, env);
        var_set(&block_env, "name", 0, true, name_obj);
        Block *attr_block = block_new(&block_env, self, Module_attr_writer_block_fn);
        define_method_with_block(env, self, method_name->str, attr_block);
    }
    return NAT_NIL;
}

Value *Module_attr_writer_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *val = args[0];
    Value *name_obj = var_get(env->outer, "name", 0);
    assert(name_obj);
    Value *ivar_name = sprintf(env, "@%S", name_obj);
    ivar_set(env, self, ivar_name->str, val);
    return val;
}

Value *Module_attr_accessor(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    Module_attr_reader(env, self, argc, args, block);
    Module_attr_writer(env, self, argc, args, block);
    return NAT_NIL;
}

Value *Module_include(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        class_include(env, self, args[i]);
    }
    return self;
}

Value *Module_prepend(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (int i = argc - 1; i >= 0; i--) {
        class_prepend(env, self, args[i]);
    }
    return self;
}

Value *Module_included_modules(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC(0);
    Value *modules = array_new(env);
    for (ssize_t i = 0; i < self->included_modules_count; i++) {
        array_push(env, modules, self->included_modules[i]);
    }
    return modules;
}

Value *Module_define_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC(1);
    Value *name_obj = args[0];
    if (NAT_TYPE(name_obj) != ValueType::String && NAT_TYPE(name_obj) != ValueType::Symbol) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    if (!block) {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
    char *name = NAT_TYPE(name_obj) == ValueType::String ? name_obj->str : name_obj->symbol;
    define_method_with_block(env, self, name, block);
    return symbol(env, name);
}

Value *Module_class_eval(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    if (argc > 0 || !block) {
        NAT_RAISE(env, "ArgumentError", "Natalie only supports class_eval with a block");
    }
    Env e;
    build_block_env(&e, &block->env, env);
    return block->fn(&e, self, 0, NULL, NULL);
}

Value *Module_private(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    printf("TODO: class private\n");
    return NAT_NIL;
}

Value *Module_protected(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    printf("TODO: class protected\n");
    return NAT_NIL;
}

Value *Module_const_defined(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC(1);
    Value *name_obj = args[0];
    if (NAT_TYPE(name_obj) == ValueType::String) {
        // we're good!
    } else if (NAT_TYPE(name_obj) == ValueType::Symbol) {
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

Value *Module_alias_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Module || NAT_TYPE(self) == ValueType::Class);
    NAT_ASSERT_ARGC(2);
    Value *new_name = args[0];
    if (NAT_TYPE(new_name) == ValueType::String) {
        // we're good!
    } else if (NAT_TYPE(new_name) == ValueType::Symbol) {
        new_name = string(env, new_name->symbol);
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", send(env, new_name, "inspect", 0, NULL, NULL));
    }
    Value *old_name = args[1];
    if (NAT_TYPE(old_name) == ValueType::String) {
        // we're good!
    } else if (NAT_TYPE(old_name) == ValueType::Symbol) {
        old_name = string(env, old_name->symbol);
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", send(env, old_name, "inspect", 0, NULL, NULL));
    }
    alias(env, self, new_name->str, old_name->str);
    return self;
}

}
