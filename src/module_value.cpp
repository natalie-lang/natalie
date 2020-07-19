#include "natalie.hpp"

namespace Natalie {

ModuleValue::ModuleValue(Env *env)
    : ModuleValue { env, Value::Type::Module, NAT_OBJECT->const_get(env, "Module", true)->as_class() } { }

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

void ModuleValue::extend(Env *env, ModuleValue *module) {
    singleton_class(env)->include(env, module);
}

void ModuleValue::include(Env *env, ModuleValue *module) {
    if (m_included_modules.is_empty()) {
        m_included_modules.push(this);
    }
    m_included_modules.push(module);
}

void ModuleValue::prepend(Env *env, ModuleValue *module) {
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
        while (!(val = static_cast<Value *>(hashmap_get(&search_parent->m_constants, name))) && search_parent->owner() && search_parent->owner() != NAT_OBJECT) {
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
        val = static_cast<Value *>(hashmap_get(&NAT_OBJECT->m_constants, name));
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
    body_env.caller = env;
    Value *result = fn(&body_env, this);
    body_env.caller = nullptr;
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
    if (method && method->undefined) {
        return nullptr;
    } else {
        return method;
    }
}

Value *ModuleValue::call_method(Env *env, Value *instance_class, const char *method_name, Value *self, ssize_t argc, Value **args, Block *block) {
    ModuleValue *matching_class_or_module;
    Method *method = find_method(method_name, &matching_class_or_module);
    if (method && !method->undefined) {
        Env *closure_env;
        if (NAT_OBJ_HAS_ENV(method)) {
            closure_env = &method->env;
        } else {
            closure_env = &matching_class_or_module->m_env;
        }
        Env e = Env::new_block_env(closure_env, env);
        e.file = env->file;
        e.line = env->line;
        e.method_name = method_name;
        e.block = block;
        return method->fn(&e, self, argc, args, block);
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

}
