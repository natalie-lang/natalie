#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct IntegerValue : Value {
    using Value::Value;

    IntegerValue(Env *env)
        : Value { env, Value::Type::Integer, const_get(env, NAT_OBJECT, "Integer", true)->as_class() } { }

    IntegerValue(Env *env, int64_t integer)
        : Value { env, Value::Type::Integer, const_get(env, NAT_OBJECT, "Integer", true)->as_class() }
        , integer { integer } { }

    int64_t integer { 0 };

    int64_t to_int64_t() {
        return integer;
    }

    bool is_zero() {
        return integer == 0;
    }
};

}
