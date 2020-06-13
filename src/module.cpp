#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Module_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return new ModuleValue { env };
}

Value *Module_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(0);
    if (self->class_name()) {
        if (self->owner && self->owner != NAT_OBJECT) {
            return StringValue::sprintf(env, "%S::%s", Module_inspect(env, self->owner, 0, NULL, NULL), self->class_name());
        } else {
            return new StringValue { env, self->class_name() };
        }
    } else if (NAT_TYPE(self) == Value::Type::Class) {
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        self->pointer_id(buf);
        return StringValue::sprintf(env, "#<Class:%s>", buf);
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
    if (self->class_name()) {
        return new StringValue { env, self->class_name() };
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
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        Env block_env = Env::new_detatched_block_env(env);
        block_env.var_set("name", 0, true, name_obj);
        Block *attr_block = block_new(&block_env, self, Module_attr_reader_block_fn);
        self->define_method_with_block(env, name_obj->as_string()->c_str(), attr_block);
    }
    return NAT_NIL;
}

Value *Module_attr_reader_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *name_obj = env->outer->var_get("name", 0);
    assert(name_obj);
    StringValue *ivar_name = StringValue::sprintf(env, "@%S", name_obj);
    return self->ivar_get(env, ivar_name->c_str());
}

Value *Module_attr_writer(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (ssize_t i = 0; i < argc; i++) {
        Value *name_obj = args[i];
        if (NAT_TYPE(name_obj) == Value::Type::String) {
            // we're good!
        } else if (NAT_TYPE(name_obj) == Value::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, name_obj, "inspect", 0, NULL, NULL));
        }
        StringValue *method_name = new StringValue { env, name_obj->as_string()->c_str() };
        method_name->append_char(env, '=');
        Env block_env = Env::new_detatched_block_env(env);
        block_env.var_set("name", 0, true, name_obj);
        Block *attr_block = block_new(&block_env, self, Module_attr_writer_block_fn);
        self->define_method_with_block(env, method_name->c_str(), attr_block);
    }
    return NAT_NIL;
}

Value *Module_attr_writer_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *val = args[0];
    Value *name_obj = env->outer->var_get("name", 0);
    assert(name_obj);
    StringValue *ivar_name = StringValue::sprintf(env, "@%S", name_obj);
    self->ivar_set(env, ivar_name->c_str(), val);
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
        self->include(env, args[i]->as_module());
    }
    return self;
}

Value *Module_prepend(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC_AT_LEAST(1);
    for (int i = argc - 1; i >= 0; i--) {
        self->prepend(env, args[i]->as_module());
    }
    return self;
}

Value *Module_included_modules(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(0);
    ArrayValue *modules = new ArrayValue { env };
    for (ModuleValue *m : self->included_modules()) {
        modules->push(m);
    }
    return modules;
}

Value *Module_define_method(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(1);
    Value *name_obj = args[0];
    const char *name = name_obj->identifier_str(env, Value::Conversion::Strict);
    if (!block) {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
    self->define_method_with_block(env, name, block);
    return SymbolValue::intern(env, name);
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
    const char *name = args[0]->identifier_str(env, Value::Conversion::NullAllowed);
    if (!name) {
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s to String", send(env, args[0], "inspect", 0, NULL, NULL));
    }
    if (self->const_get_or_null(env, name, false, false)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Module_alias_method(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ModuleValue *self = self_value->as_module();
    NAT_ASSERT_ARGC(2);
    const char *new_name = args[0]->identifier_str(env, Value::Conversion::NullAllowed);
    if (!new_name) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", send(env, args[0], "inspect", 0, NULL, NULL));
    }
    const char *old_name = args[1]->identifier_str(env, Value::Conversion::NullAllowed);
    if (!old_name) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", send(env, args[1], "inspect", 0, NULL, NULL));
    }
    self->alias(env, new_name, old_name);
    return self;
}

}
