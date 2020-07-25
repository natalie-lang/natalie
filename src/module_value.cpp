#include "natalie.hpp"

namespace Natalie {

ModuleValue::ModuleValue(Env *env)
    : ModuleValue { env, Value::Type::Module, env->Object()->const_get(env, "Module", true)->as_class() } { }

ModuleValue::ModuleValue(Env *env, const char *name)
    : ModuleValue { env } {
    this->set_class_name(name);
}

ModuleValue::ModuleValue(Env *env, Type type, ClassValue *klass)
    : Value { type, klass } {
    m_env = Env::new_detatched_block_env(env);
    hashmap_init(&m_methods, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&m_methods, hashmap_alloc_key_string, free);
    hashmap_init(&m_constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&m_constants, hashmap_alloc_key_string, free);
}

Value *ModuleValue::extend(Env *env, ssize_t argc, Value **args) {
    for (ssize_t i = 0; i < argc; i++) {
        extend_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleValue::extend_once(Env *env, ModuleValue *module) {
    singleton_class(env)->include_once(env, module);
}

Value *ModuleValue::include(Env *env, ssize_t argc, Value **args) {
    for (ssize_t i = 0; i < argc; i++) {
        include_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleValue::include_once(Env *env, ModuleValue *module) {
    if (m_included_modules.is_empty()) {
        m_included_modules.push(this);
    }
    m_included_modules.push(module);
}

Value *ModuleValue::prepend(Env *env, ssize_t argc, Value **args) {
    for (int i = argc - 1; i >= 0; i--) {
        prepend_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleValue::prepend_once(Env *env, ModuleValue *module) {
    if (m_included_modules.is_empty()) {
        m_included_modules.push(this);
    }
    m_included_modules.push_front(module);
}

Value *ModuleValue::const_get(Env *env, const char *name, bool strict) {
    Value *val = const_get_or_null(env, name, strict, false);
    if (val) {
        return val;
    } else if (strict) {
        NAT_RAISE(env, "NameError", "uninitialized constant %S::%s", this->send(env, "inspect", 0, nullptr, nullptr), name);
    } else {
        NAT_RAISE(env, "NameError", "uninitialized constant %s", name);
    }
}

Value *ModuleValue::const_get_or_null(Env *env, const char *name, bool strict, bool define) {
    ModuleValue *search_parent;
    Value *val;

    if (!strict) {
        // first search in parent namespaces (not including global, i.e. Object namespace)
        search_parent = this;
        while (!(val = static_cast<Value *>(hashmap_get(&search_parent->m_constants, name))) && search_parent->owner() && search_parent->owner() != env->Object()) {
            search_parent = search_parent->owner();
        }
        if (val) return val;
    }

    if (define) {
        // don't search superclasses
        val = static_cast<Value *>(hashmap_get(&m_constants, name));
        if (val) return val;
    } else {
        // search in superclass hierarchy
        search_parent = this;
        while (!(val = static_cast<Value *>(hashmap_get(&search_parent->m_constants, name))) && search_parent->m_superclass) {
            search_parent = search_parent->m_superclass;
        }
        if (val) return val;
    }

    if (!strict) {
        // lastly, search on the global, i.e. Object namespace
        val = static_cast<Value *>(hashmap_get(&env->Object()->m_constants, name));
        if (val) return val;
    }

    return nullptr;
}

Value *ModuleValue::const_set(Env *env, const char *name, Value *val) {
    hashmap_remove(&m_constants, name);
    hashmap_put(&m_constants, name, val);
    if (val->is_module() && !val->owner()) {
        val->set_owner(this);
    }
    return val;
}

void ModuleValue::alias(Env *env, const char *new_name, const char *old_name) {
    ModuleValue *matching_class_or_module;
    Method *method = find_method(old_name, &matching_class_or_module);
    if (!method) {
        NAT_RAISE(env, "NameError", "undefined method `%s' for `%v'", old_name, this);
    }
    free(hashmap_remove(&m_methods, new_name));
    hashmap_put(&m_methods, new_name, new Method { *method });
}

Value *ModuleValue::eval_body(Env *env, Value *(*fn)(Env *, Value *)) {
    Env body_env = new Env { env };
    body_env.set_caller(env);
    Value *result = fn(&body_env, this);
    body_env.clear_caller();
    return result;
}

Value *ModuleValue::cvar_get_or_null(Env *env, const char *name) {
    ModuleValue *module = this;
    Value *val = nullptr;
    while (1) {
        if (module->m_class_vars.table) {
            val = static_cast<Value *>(hashmap_get(&module->m_class_vars, name));
            if (val) {
                return val;
            }
        }
        if (!module->m_superclass) {
            return nullptr;
        }
        module = module->m_superclass;
    }
}

Value *ModuleValue::cvar_set(Env *env, const char *name, Value *val) {
    ModuleValue *current = this;

    Value *exists = nullptr;
    while (1) {
        if (current->m_class_vars.table) {
            exists = static_cast<Value *>(hashmap_get(&current->m_class_vars, name));
            if (exists) {
                hashmap_remove(&current->m_class_vars, name);
                hashmap_put(&current->m_class_vars, name, val);
                return val;
            }
        }
        if (!current->m_superclass) {
            if (this->m_class_vars.table == nullptr) {
                hashmap_init(&this->m_class_vars, hashmap_hash_string, hashmap_compare_string, 10);
                hashmap_set_key_alloc_funcs(&this->m_class_vars, hashmap_alloc_key_string, free);
            }
            hashmap_remove(&this->m_class_vars, name);
            hashmap_put(&this->m_class_vars, name, val);
            return val;
        }
        current = current->m_superclass;
    }
}

void ModuleValue::define_method(Env *env, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block)) {
    Method *method = new Method { fn };
    free(hashmap_remove(&m_methods, name));
    hashmap_put(&m_methods, name, method);
}

void ModuleValue::define_method_with_block(Env *env, const char *name, Block *block) {
    Method *method = new Method { block };
    free(hashmap_remove(&m_methods, name));
    hashmap_put(&m_methods, name, method);
}

void ModuleValue::undefine_method(Env *env, const char *name) {
    define_method(env, name, nullptr);
}

// supply an empty array and it will be populated with the method names as symbols
void ModuleValue::methods(Env *env, ArrayValue *array) {
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(&m_methods); iter; iter = hashmap_iter_next(&m_methods, iter)) {
        const char *name = (char *)hashmap_iter_get_key(iter);
        array->push(SymbolValue::intern(env, name));
    }
    for (ModuleValue *module : m_included_modules) {
        for (iter = hashmap_iter(&module->m_methods); iter; iter = hashmap_iter_next(&module->m_methods, iter)) {
            const char *name = (char *)hashmap_iter_get_key(iter);
            array->push(SymbolValue::intern(env, name));
        }
    }
    if (m_superclass) {
        return m_superclass->methods(env, array);
    }
}

// returns the method and sets matching_class_or_module to where the method was found
Method *ModuleValue::find_method(const char *method_name, ModuleValue **matching_class_or_module) {
    Method *method;
    if (m_included_modules.is_empty()) {
        // no included modules, just search the class/module
        // note: if there are included modules, then the module chain will include this class/module
        method = static_cast<Method *>(hashmap_get(&m_methods, method_name));
        if (method) {
            *matching_class_or_module = m_klass;
            return method;
        }
    }

    for (ModuleValue *module : m_included_modules) {
        method = static_cast<Method *>(hashmap_get(&module->m_methods, method_name));
        if (method) {
            *matching_class_or_module = module;
            return method;
        }
    }

    if (m_superclass) {
        return m_superclass->find_method(method_name, matching_class_or_module);
    } else {
        return nullptr;
    }
}

Method *ModuleValue::find_method_without_undefined(const char *method_name, ModuleValue **matching_class_or_module) {
    Method *method = find_method(method_name, matching_class_or_module);
    if (method && method->is_undefined()) {
        return nullptr;
    } else {
        return method;
    }
}

Value *ModuleValue::call_method(Env *env, Value *instance_class, const char *method_name, Value *self, ssize_t argc, Value **args, Block *block) {
    ModuleValue *matching_class_or_module;
    Method *method = find_method(method_name, &matching_class_or_module);
    if (method && !method->is_undefined()) {
        Env *closure_env;
        if (method->has_env()) {
            closure_env = method->env();
        } else {
            closure_env = &matching_class_or_module->m_env;
        }
        Env e = Env::new_block_env(closure_env, env);
        e.set_file(env->file());
        e.set_line(env->line());
        e.set_method_name(method_name);
        e.set_block(block);
        return method->run(&e, self, argc, args, block);
    } else {
        NAT_RAISE(env, "NoMethodError", "undefined method `%s' for %v", method_name, instance_class);
    }
}

ArrayValue *ModuleValue::ancestors(Env *env) {
    ModuleValue *klass = this;
    ArrayValue *ancestors = new ArrayValue { env };
    do {
        if (klass->included_modules().is_empty()) {
            // note: if there are included modules, then they will include this klass
            ancestors->push(klass);
        }
        for (ModuleValue *m : klass->included_modules()) {
            ancestors->push(m);
        }
        klass = klass->superclass();
    } while (klass);
    return ancestors;
}

bool ModuleValue::is_method_defined(Env *env, Value *name_value) {
    const char *name = name_value->identifier_str(env, Conversion::Strict);
    ModuleValue *matching_class_or_module;
    return !!find_method(name, &matching_class_or_module);
}

Value *ModuleValue::inspect(Env *env) {
    if (m_class_name) {
        if (owner() && owner() != env->Object()) {
            return StringValue::sprintf(env, "%S::%s", owner()->send(env, "inspect"), m_class_name);
        } else {
            return new StringValue { env, m_class_name };
        }
    } else if (is_class()) {
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        pointer_id(buf);
        return StringValue::sprintf(env, "#<Class:%s>", buf);
    } else if (is_module() && m_class_name) {
        return new StringValue { env, m_class_name };
    } else {
        // TODO: extract this somewhere?
        StringValue *str = new StringValue { env, "#<" };
        StringValue *inspected = klass()->send(env, "inspect")->as_string();
        str->append_string(env, inspected);
        str->append_char(env, ':');
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        pointer_id(buf);
        str->append(env, buf);
        str->append_char(env, '>');
        return str;
    }
}

Value *ModuleValue::name(Env *env) {
    if (m_class_name) {
        return new StringValue { env, m_class_name };
    } else {
        return env->nil_obj();
    }
}

Value *ModuleValue::attr_reader(Env *env, ssize_t argc, Value **args) {
    for (ssize_t i = 0; i < argc; i++) {
        Value *name_obj = args[i];
        if (name_obj->type() == Value::Type::String) {
            // we're good!
        } else if (name_obj->type() == Value::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", name_obj->send(env, "inspect"));
        }
        Env block_env = Env::new_detatched_block_env(env);
        block_env.var_set("name", 0, true, name_obj);
        Block *attr_block = new Block { block_env, this, ModuleValue::attr_reader_block_fn };
        define_method_with_block(env, name_obj->as_string()->c_str(), attr_block);
    }
    return env->nil_obj();
}

Value *ModuleValue::attr_reader_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *name_obj = env->outer()->var_get("name", 0);
    assert(name_obj);
    StringValue *ivar_name = StringValue::sprintf(env, "@%S", name_obj);
    return self->ivar_get(env, ivar_name->c_str());
}

Value *ModuleValue::attr_writer(Env *env, ssize_t argc, Value **args) {
    for (ssize_t i = 0; i < argc; i++) {
        Value *name_obj = args[i];
        if (name_obj->type() == Value::Type::String) {
            // we're good!
        } else if (name_obj->type() == Value::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", name_obj->send(env, "inspect"));
        }
        StringValue *method_name = new StringValue { env, name_obj->as_string()->c_str() };
        method_name->append_char(env, '=');
        Env block_env = Env::new_detatched_block_env(env);
        block_env.var_set("name", 0, true, name_obj);
        Block *attr_block = new Block { block_env, this, ModuleValue::attr_writer_block_fn };
        define_method_with_block(env, method_name->c_str(), attr_block);
    }
    return env->nil_obj();
}

Value *ModuleValue::attr_writer_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *val = args[0];
    Value *name_obj = env->outer()->var_get("name", 0);
    assert(name_obj);
    StringValue *ivar_name = StringValue::sprintf(env, "@%S", name_obj);
    self->ivar_set(env, ivar_name->c_str(), val);
    return val;
}

Value *ModuleValue::attr_accessor(Env *env, ssize_t argc, Value **args) {
    attr_reader(env, argc, args);
    attr_writer(env, argc, args);
    return env->nil_obj();
}

Value *ModuleValue::included_modules(Env *env) {
    ArrayValue *modules = new ArrayValue { env };
    for (ModuleValue *m : included_modules()) {
        modules->push(m);
    }
    return modules;
}

Value *ModuleValue::define_method(Env *env, Value *name_value, Block *block) {
    const char *name = name_value->identifier_str(env, Value::Conversion::Strict);
    if (!block) {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
    define_method_with_block(env, name, block);
    return SymbolValue::intern(env, name);
}

Value *ModuleValue::class_eval(Env *env, Block *block) {
    if (!block) {
        NAT_RAISE(env, "ArgumentError", "Natalie only supports class_eval with a block");
    }
    block->set_self(this);
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
    return env->nil_obj();
}

Value *ModuleValue::private_method(Env *env, Value *method_name) {
    printf("TODO: class private\n");
    return env->nil_obj();
}

Value *ModuleValue::protected_method(Env *env, Value *method_name) {
    printf("TODO: class protected\n");
    return env->nil_obj();
}

Value *ModuleValue::public_method(Env *env, Value *method_name) {
    printf("TODO: class public\n");
    return env->nil_obj();
}

bool ModuleValue::const_defined(Env *env, Value *name_value) {
    const char *name = name_value->identifier_str(env, Value::Conversion::NullAllowed);
    if (!name) {
        NAT_RAISE(env, "TypeError", "no implicit conversion of %v to String", name_value);
    }
    return !!const_get_or_null(env, name, false, false);
}

Value *ModuleValue::alias_method(Env *env, Value *new_name_value, Value *old_name_value) {
    const char *new_name = new_name_value->identifier_str(env, Value::Conversion::NullAllowed);
    if (!new_name) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", new_name_value->send(env, "inspect"));
    }
    const char *old_name = old_name_value->identifier_str(env, Value::Conversion::NullAllowed);
    if (!old_name) {
        NAT_RAISE(env, "TypeError", "%s is not a symbol", old_name_value->send(env, "inspect"));
    }
    alias(env, new_name, old_name);
    return this;
}

}
