#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct IntegerValue : Value {
    IntegerValue(Env *env, int64_t integer)
        : Value { Value::Type::Integer, NAT_OBJECT->const_get(env, "Integer", true)->as_class() }
        , m_integer { integer } { }

    int64_t to_int64_t() {
        return m_integer;
    }

    bool is_zero() {
        return m_integer == 0;
    }

private:
    int64_t m_integer { 0 };
};

}
