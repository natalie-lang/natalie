#include "natalie.hpp"

namespace Natalie {

ModuleValue::ModuleValue(Env *env)
    : ModuleValue { env, Value::Type::Module, const_get(env, NAT_OBJECT, "Module", true)->as_class() } { }

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

}
