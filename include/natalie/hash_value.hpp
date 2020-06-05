#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct HashValue : Value {
    using Value::Value;

    HashValue(Env *env)
        : Value { env, Value::Type::Hash, const_get(env, NAT_OBJECT, "Hash", true)->as_class() } { }

    HashKey *key_list { nullptr };
    struct hashmap hashmap EMPTY_HASHMAP;
    bool hash_is_iterating { false };
    Value *hash_default_value { nullptr };
    Block *hash_default_block { nullptr };
};

}
