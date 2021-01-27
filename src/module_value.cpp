#include "natalie.hpp"

namespace Natalie {

ModuleValue::ModuleValue(Env *env)
    : ModuleValue { env, Value::Type::Module, env->Object()->const_find(env, "Module")->as_class() } { }

ModuleValue::ModuleValue(Env *env, const char *name)
    : ModuleValue { env } {
    this->set_class_name(name);
}

ModuleValue::ModuleValue(Env *env, Type type, ClassValue *klass)
    : Value { type, klass } {
    m_env = Env::new_detatched_env(env);
    hashmap_init(&m_methods, hashmap_hash_ptr, hashmap_compare_ptr, 10);
    hashmap_set_key_alloc_funcs(&m_methods, nullptr, nullptr);
    hashmap_init(&m_constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&m_constants, hashmap_alloc_key_string, nullptr);
}

ValuePtr ModuleValue::extend(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        extend_once(env, args[i]->as_module());
    }
    return this;
}

void ModuleValue::extend_once(Env *env, ModuleValue *module) {
    singleton_class(env)->include_once(env, module);
}

ValuePtr ModuleValue::include(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
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

ValuePtr ModuleValue::prepend(Env *env, size_t argc, ValuePtr *args) {
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

ValuePtr ModuleValue::const_get(Env *env, const char *name) {
    return static_cast<ValuePtr>(hashmap_get(env, &m_constants, name));
}

ValuePtr ModuleValue::const_fetch(Env *env, const char *name) {
    ValuePtr value = const_get(env, name);
    if (!value) {
        printf("Constant %s is missing!\n", name);
        abort();
    }
    return value;
}

ValuePtr ModuleValue::const_find(Env *env, const char *name, ConstLookupSearchMode search_mode, ConstLookupFailureMode failure_mode) {
    ModuleValue *search_parent;
    ValuePtr val;

    if (search_mode == ConstLookupSearchMode::NotStrict) {
        // first search in parent namespaces (not including global, i.e. Object namespace)
        search_parent = this;
        while (!(val = search_parent->const_get(env, name)) && search_parent->owner() && search_parent->owner() != env->Object()) {
            search_parent = search_parent->owner();
        }
        if (val) return val;
    }

    // search in superclass hierarchy
    search_parent = this;
    do {
        val = search_parent->const_get(env, name);
        if (val) return val;
        search_parent = search_parent->m_superclass;
    } while (search_parent);

    if (this != env->Object() && search_mode == ConstLookupSearchMode::NotStrict) {
        // lastly, search on the global, i.e. Object namespace
        val = env->Object()->const_get(env, name);
        if (val) return val;
    }

    if (failure_mode == ConstLookupFailureMode::Null) return nullptr;

    if (search_mode == ConstLookupSearchMode::Strict) {
        env->raise("NameError", "uninitialized constant %S::%s", this->send(env, "inspect", 0, nullptr, nullptr), name);
    } else {
        env->raise("NameError", "uninitialized constant %s", name);
    }
}

ValuePtr ModuleValue::const_set(Env *env, const char *name, ValuePtr val) {
    hashmap_remove(env, &m_constants, name);
    hashmap_put(env, &m_constants, name, val);
    if (val->is_module() && !val->owner()) {
        val->set_owner(this);
        if (val->singleton_class()) val->singleton_class()->set_owner(this);
    }
    return val;
}

void ModuleValue::alias(Env *env, SymbolValue *new_name, SymbolValue *old_name) {
    Method *method = find_method(env, old_name);
    if (!method) {
        env->raise("NameError", "undefined method `%s' for `%v'", old_name->c_str(), this);
    }
    GC_FREE(hashmap_remove(env, &m_methods, new_name));
    hashmap_put(env, &m_methods, new_name, new Method { *method });
}

ValuePtr ModuleValue::eval_body(Env *env, ValuePtr (*fn)(Env *, ValuePtr)) {
    Env body_env = Env { env };
    body_env.set_caller(env);
    ValuePtr result = fn(&body_env, this);
    body_env.clear_caller();
    return result;
}

ValuePtr ModuleValue::cvar_get_or_null(Env *env, const char *name) {
    ModuleValue *module = this;
    ValuePtr val = nullptr;
    while (1) {
        if (module->m_class_vars.table) {
            val = static_cast<ValuePtr>(hashmap_get(env, &module->m_class_vars, name));
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

ValuePtr ModuleValue::cvar_set(Env *env, const char *name, ValuePtr val) {
    ModuleValue *current = this;

    ValuePtr exists = nullptr;
    while (1) {
        if (current->m_class_vars.table) {
            exists = static_cast<ValuePtr>(hashmap_get(env, &current->m_class_vars, name));
            if (exists) {
                hashmap_remove(env, &current->m_class_vars, name);
                hashmap_put(env, &current->m_class_vars, name, val);
                return val;
            }
        }
        if (!current->m_superclass) {
            if (this->m_class_vars.table == nullptr) {
                hashmap_init(&this->m_class_vars, hashmap_hash_string, hashmap_compare_string, 10);
                hashmap_set_key_alloc_funcs(&this->m_class_vars, hashmap_alloc_key_string, nullptr);
            }
            hashmap_remove(env, &this->m_class_vars, name);
            hashmap_put(env, &this->m_class_vars, name, val);
            return val;
        }
        current = current->m_superclass;
    }
}

SymbolValue *ModuleValue::define_method(Env *env, SymbolValue *name, MethodFnPtr fn) {
    Method *method = new Method { name->c_str(), this, fn };
    GC_FREE(hashmap_remove(env, &m_methods, name));
    hashmap_put(env, &m_methods, name, method);
    return name;
}

SymbolValue *ModuleValue::define_method_with_block(Env *env, SymbolValue *name, Block *block) {
    Method *method = new Method { name->c_str(), this, block };
    GC_FREE(hashmap_remove(env, &m_methods, name));
    hashmap_put(env, &m_methods, name, method);
    return name;
}

SymbolValue *ModuleValue::undefine_method(Env *env, SymbolValue *name) {
    return define_method(env, name, nullptr);
}

// supply an empty array and it will be populated with the method names as symbols
void ModuleValue::methods(Env *env, ArrayValue *array) {
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(&m_methods); iter; iter = hashmap_iter_next(&m_methods, iter)) {
        auto name = (SymbolValue *)hashmap_iter_get_key(iter);
        array->push(name);
    }
    for (ModuleValue *module : m_included_modules) {
        for (iter = hashmap_iter(&module->m_methods); iter; iter = hashmap_iter_next(&module->m_methods, iter)) {
            auto name = (SymbolValue *)hashmap_iter_get_key(iter);
            array->push(name);
        }
    }
    if (m_superclass) {
        return m_superclass->methods(env, array);
    }
}

// returns the method and sets matching_class_or_module to where the method was found
Method *ModuleValue::find_method(Env *env, SymbolValue *method_name, ModuleValue **matching_class_or_module) {
    Method *method;
    if (m_included_modules.is_empty()) {
        // no included modules, just search the class/module
        // note: if there are included modules, then the module chain will include this class/module
        method = static_cast<Method *>(hashmap_get(env, &m_methods, method_name));
        if (method) {
            if (matching_class_or_module) *matching_class_or_module = m_klass;
            return method;
        }
    }

    for (ModuleValue *module : m_included_modules) {
        method = static_cast<Method *>(hashmap_get(env, &module->m_methods, method_name));
        if (method) {
            if (matching_class_or_module) *matching_class_or_module = module;
            return method;
        }
    }

    if (m_superclass) {
        return m_superclass->find_method(env, method_name, matching_class_or_module);
    } else {
        return nullptr;
    }
}

Method *ModuleValue::find_method(Env *env, const char *method_name, ModuleValue **matching_class_or_module) {
    return find_method(env, SymbolValue::intern(env, method_name), matching_class_or_module);
}

ValuePtr ModuleValue::call_method(Env *env, ValuePtr instance_class, SymbolValue *method_name, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
    Method *method = find_method(env, method_name);
    if (method && !method->is_undefined()) {
        return method->call(env, self, argc, args, block);
    } else if (self->is_module()) {
        env->raise("NoMethodError", "undefined method `%s' for %s:%v", method_name->c_str(), self->as_module()->class_name(), instance_class);
    } else if (method_name == SymbolValue::intern(env, "inspect")) {
        env->raise("NoMethodError", "undefined method `inspect' for #<%s:0x%x>", self->klass()->class_name(), self->object_id());
    } else {
        env->raise("NoMethodError", "undefined method `%s' for %s", method_name->c_str(), self->inspect_str(env));
    }
}

ValuePtr ModuleValue::call_method(Env *env, ValuePtr instance_class, const char *method_name, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
    return call_method(env, instance_class, SymbolValue::intern(env, method_name), self, argc, args, block);
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

bool ModuleValue::is_method_defined(Env *env, ValuePtr name_value) {
    const char *name = name_value->identifier_str(env, Conversion::Strict);
    return !!find_method(env, name);
}

ValuePtr ModuleValue::inspect(Env *env) {
    if (m_class_name) {
        if (owner() && owner() != env->Object()) {
            return StringValue::sprintf(env, "%S::%s", owner()->send(env, "inspect"), m_class_name);
        } else {
            return new StringValue { env, m_class_name };
        }
    } else if (is_class()) {
        return StringValue::sprintf(env, "#<Class:%s>", pointer_id());
    } else if (is_module() && m_class_name) {
        return new StringValue { env, m_class_name };
    } else {
        // TODO: extract this somewhere?
        StringValue *str = new StringValue { env, "#<" };
        StringValue *inspected = klass()->send(env, "inspect")->as_string();
        str->append_string(env, inspected);
        str->append_char(env, ':');
        str->append(env, pointer_id());
        str->append_char(env, '>');
        return str;
    }
}

ValuePtr ModuleValue::name(Env *env) {
    if (m_class_name) {
        return new StringValue { env, m_class_name };
    } else {
        return env->nil_obj();
    }
}

ValuePtr ModuleValue::attr_reader(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        ValuePtr name_obj = args[i];
        if (name_obj->type() == Value::Type::String) {
            // we're good!
        } else if (name_obj->type() == Value::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            env->raise("TypeError", "%s is not a symbol nor a string", name_obj->send(env, "inspect"));
        }
        Env block_env = Env::new_detatched_env(env);
        block_env.var_set("name", 0, true, name_obj);
        Block *attr_block = new Block { block_env, this, ModuleValue::attr_reader_block_fn };
        define_method_with_block(env, name_obj->as_string()->to_symbol(env), attr_block);
    }
    return env->nil_obj();
}

ValuePtr ModuleValue::attr_reader_block_fn(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
    ValuePtr name_obj = env->outer()->var_get("name", 0);
    assert(name_obj);
    StringValue *ivar_name = StringValue::sprintf(env, "@%S", name_obj);
    return self->ivar_get(env, ivar_name->c_str());
}

ValuePtr ModuleValue::attr_writer(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        ValuePtr name_obj = args[i];
        if (name_obj->type() == Value::Type::String) {
            // we're good!
        } else if (name_obj->type() == Value::Type::Symbol) {
            name_obj = name_obj->as_symbol()->to_s(env);
        } else {
            env->raise("TypeError", "%s is not a symbol nor a string", name_obj->send(env, "inspect"));
        }
        StringValue *method_name = new StringValue { env, name_obj->as_string()->c_str() };
        method_name->append_char(env, '=');
        Env block_env = Env::new_detatched_env(env);
        block_env.var_set("name", 0, true, name_obj);
        Block *attr_block = new Block { block_env, this, ModuleValue::attr_writer_block_fn };
        define_method_with_block(env, method_name->to_symbol(env), attr_block);
    }
    return env->nil_obj();
}

ValuePtr ModuleValue::attr_writer_block_fn(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
    ValuePtr val = args[0];
    ValuePtr name_obj = env->outer()->var_get("name", 0);
    assert(name_obj);
    StringValue *ivar_name = StringValue::sprintf(env, "@%S", name_obj);
    self->ivar_set(env, ivar_name->c_str(), val);
    return val;
}

ValuePtr ModuleValue::attr_accessor(Env *env, size_t argc, ValuePtr *args) {
    attr_reader(env, argc, args);
    attr_writer(env, argc, args);
    return env->nil_obj();
}

ValuePtr ModuleValue::included_modules(Env *env) {
    ArrayValue *modules = new ArrayValue { env };
    for (ModuleValue *m : included_modules()) {
        modules->push(m);
    }
    return modules;
}

bool ModuleValue::does_include_module(Env *env, ValuePtr module) {
    module->assert_type(env, Value::Type::Module, "Module");
    for (ModuleValue *m : included_modules()) {
        if (m == this) continue;
        if (m == module) return true;
    }
    return false;
}

ValuePtr ModuleValue::define_method(Env *env, ValuePtr name_value, Block *block) {
    auto name = name_value->to_symbol(env, Value::Conversion::Strict);
    if (!block) {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
    define_method_with_block(env, name, block);
    return name;
}

ValuePtr ModuleValue::module_eval(Env *env, Block *block) {
    if (!block) {
        env->raise("ArgumentError", "Natalie only supports module_eval with a block");
    }
    block->set_self(this);
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
    return env->nil_obj();
}

ValuePtr ModuleValue::private_method(Env *env, ValuePtr method_name) {
    // TODO: class private
    return env->nil_obj();
}

ValuePtr ModuleValue::protected_method(Env *env, ValuePtr method_name) {
    // TODO: class protected
    return env->nil_obj();
}

ValuePtr ModuleValue::public_method(Env *env, ValuePtr method_name) {
    // TODO: class public
    return env->nil_obj();
}

bool ModuleValue::const_defined(Env *env, ValuePtr name_value) {
    const char *name = name_value->identifier_str(env, Value::Conversion::NullAllowed);
    if (!name) {
        env->raise("TypeError", "no implicit conversion of %v to String", name_value);
    }
    return !!const_find(env, name, ConstLookupSearchMode::NotStrict, ConstLookupFailureMode::Null);
}

ValuePtr ModuleValue::alias_method(Env *env, ValuePtr new_name_value, ValuePtr old_name_value) {
    auto new_name = new_name_value->to_symbol(env, Value::Conversion::NullAllowed);
    if (!new_name) {
        env->raise("TypeError", "%s is not a symbol", new_name_value->send(env, "inspect"));
    }
    auto old_name = old_name_value->to_symbol(env, Value::Conversion::NullAllowed);
    if (!old_name) {
        env->raise("TypeError", "%s is not a symbol", old_name_value->send(env, "inspect"));
    }
    alias(env, new_name, old_name);
    return this;
}

}
