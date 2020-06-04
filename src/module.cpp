#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Module_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return module(env, NULL);
}

Value *Module_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(0);
    if (self->class_name) {
        if (self->owner && self->owner != NAT_OBJECT) {
            return sprintf(env, "%S::%s", Module_inspect(env, self->owner, 0, NULL, NULL), self->class_name);
        } else {
            return string(env, self->class_name);
        }
    } else if (NAT_TYPE(self) == Value::Type::Class) {
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        object_pointer_id(self, buf);
        return sprintf(env, "#<Class:%s>", buf);
    } else {
        return Kernel_inspect(env, self, argc, args, block);
    }
}

Value *Module_eqeqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (is_a(env, arg, self)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Module_name(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(0);
    if (self->class_name) {
        return string(env, self->class_name);
    } else {
        return NAT_NIL;
    }
}

Value *Module_ancestors(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(0);
    return class_ancestors(env, self);
}

Value *Module_attr_reader(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        Value *name_obj = args[i];
        if (NAT_TYPE(name_obj) == Value::Type::String) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == Value::Type::Symbol) {
            name_obj = string(env, name_obj->as_symbol()->symbol);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        Env block_env = Env::new_detatched_block_env(env);
        var_set(&block_env, "name", 0, true, name_obj);
        Block *attr_block = block_new(&block_env, self, Module_attr_reader_block_fn);
        define_method_with_block(env, self, name_obj->as_string()->str, attr_block);
    }
    return NAT_NIL;
}

Value *Module_attr_reader_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *name_obj = var_get(env->outer, "name", 0);
    assert(name_obj);
    StringValue *ivar_name = sprintf(env, "@%S", name_obj);
    return ivar_get(env, self, ivar_name->str);
}

Value *Module_attr_writer(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        Value *name_obj = args[i];
        if (NAT_TYPE(name_obj) == Value::Type::String) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == Value::Type::Symbol) {
            name_obj = string(env, name_obj->as_symbol()->symbol);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        StringValue *method_name = string(env, name_obj->as_string()->str);
        string_append_char(env, method_name, '=');
        Env block_env = Env::new_detatched_block_env(env);
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
    StringValue *ivar_name = sprintf(env, "@%S", name_obj);
    ivar_set(env, self, ivar_name->str, val);
    return val;
}

Value *Module_attr_accessor(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC_AT_LEAST(1);
    Module_attr_reader(env, self, argc, args, block);
    Module_attr_writer(env, self, argc, args, block);
    return NAT_NIL;
}

Value *Module_include(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        class_include(env, self, args[i]->as_module());
    }
    return self;
}

Value *Module_prepend(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (int i = argc - 1; i >= 0; i--) {
        class_prepend(env, self, args[i]->as_module());
    }
    return self;
}

Value *Module_included_modules(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(0);
    Value *modules = array_new(env);
    for (ssize_t i = 0; i < self->included_modules_count; i++) {
        array_push(env, modules, self->included_modules[i]);
    }
    return modules;
}

Value *Module_define_method(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(1);
    Value *name_obj = args[0];
    if (NAT_TYPE(name_obj) != Value::Type::String && NAT_TYPE(name_obj) != Value::Type::Symbol) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
    }
    if (!block) {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
    const char *name = name_obj->symbol_or_string_to_str();
    assert(name);
    define_method_with_block(env, self, name, block);
    return symbol(env, name);
}

Value *Module_class_eval(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    if (argc > 0 || !block) {
        NAT_RAISE(env, "ArgumentError", "Natalie only supports class_eval with a block");
    }
    Env e = Env::new_block_env(&block->env, env);
    return block->fn(&e, self, 0, NULL, NULL);
}

Value *Module_private(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    printf("TODO: class private\n");
    return NAT_NIL;
}

Value *Module_protected(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    printf("TODO: class protected\n");
    return NAT_NIL;
}

Value *Module_const_defined(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(1);
    const char *name = args[0]->symbol_or_string_to_str();
    if (!name) {
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s to String", send(env, args[0], "inspect", 0, NULL, NULL));
    }
    if (const_get_or_null(env, self, name, false, false)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Module_alias_method(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(2);
    const char *new_name = args[0]->symbol_or_string_to_str();
    if (!new_name) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", send(env, args[0], "inspect", 0, NULL, NULL));
    }
    const char *old_name = args[1]->symbol_or_string_to_str();
    if (!old_name) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", send(env, args[1], "inspect", 0, NULL, NULL));
    }
    alias(env, self, new_name, old_name);
    return self;
}

}
