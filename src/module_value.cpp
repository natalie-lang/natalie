#include "natalie.hpp"

namespace Natalie {

ModuleValue::ModuleValue(Env *env)
    : ModuleValue { env, Value::Type::Module, NAT_OBJECT->const_get(env, "Module", true)->as_class() } { }

ModuleValue::ModuleValue(Env *env, const char *name)
    : ModuleValue { env } {
    this->class_name = name ? strdup(name) : nullptr;
}

ModuleValue::ModuleValue(Env *env, Type type, ClassValue *klass)
    : Value { env, type, klass } {
    this->env = Env::new_detatched_block_env(env);
    hashmap_init(&this->methods, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&this->methods, hashmap_alloc_key_string, free);
    hashmap_init(&this->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&this->constants, hashmap_alloc_key_string, free);
    this->cvars.table = nullptr;
}

void ModuleValue::include(Env *env, ModuleValue *module) {
    this->included_modules_count++;
    if (this->included_modules_count == 1) {
        this->included_modules_count++;
        this->included_modules = static_cast<ModuleValue **>(calloc(2, sizeof(Value *)));
        this->included_modules[0] = this;
    } else {
        this->included_modules = static_cast<ModuleValue **>(realloc(this->included_modules, sizeof(Value *) * this->included_modules_count));
    }
    this->included_modules[this->included_modules_count - 1] = module;
}

void ModuleValue::prepend(Env *env, ModuleValue *module) {
    this->included_modules_count++;
    if (this->included_modules_count == 1) {
        this->included_modules_count++;
        this->included_modules = static_cast<ModuleValue **>(calloc(2, sizeof(Value *)));
        this->included_modules[1] = this;
    } else {
        this->included_modules = static_cast<ModuleValue **>(realloc(this->included_modules, sizeof(Value *) * this->included_modules_count));
        for (ssize_t i = this->included_modules_count - 1; i > 0; i--) {
            this->included_modules[i] = this->included_modules[i - 1];
        }
    }
    this->included_modules[0] = module;
}

Value *ModuleValue::const_get(Env *env, const char *name, bool strict) {
    Value *val = this->const_get_or_null(env, name, strict, false);
    if (val) {
        return val;
    } else if (strict) {
        NAT_RAISE(env, "NameError", "uninitialized constant %S::%s", send(env, this, "inspect", 0, nullptr, nullptr), name);
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
        while (!(val = static_cast<Value *>(hashmap_get(&search_parent->constants, name))) && search_parent->owner && search_parent->owner != NAT_OBJECT) {
            search_parent = search_parent->owner;
        }
        if (val) return val;
    }

    if (define) {
        // don't search superclasses
        val = static_cast<Value *>(hashmap_get(&this->constants, name));
        if (val) return val;
    } else {
        // search in superclass hierarchy
        search_parent = this;
        while (!(val = static_cast<Value *>(hashmap_get(&search_parent->constants, name))) && search_parent->superclass) {
            search_parent = search_parent->superclass;
        }
        if (val) return val;
    }

    if (!strict) {
        // lastly, search on the global, i.e. Object namespace
        val = static_cast<Value *>(hashmap_get(&NAT_OBJECT->constants, name));
        if (val) return val;
    }

    return nullptr;
}

Value *ModuleValue::const_set(Env *env, const char *name, Value *val) {
    hashmap_remove(&this->constants, name);
    hashmap_put(&this->constants, name, val);
    if (val->is_module() && !val->owner) {
        val->owner = this;
    }
    return val;
}

}
