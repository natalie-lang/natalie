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

}
