#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct FloatValue : Value {
    FloatValue(Env *env, double number)
        : Value { Value::Type::Float, NAT_OBJECT->const_get(env, "Float", true)->as_class() }
        , m_float { number } { }

    FloatValue(Env *env, int64_t number)
        : Value { Value::Type::Float, NAT_OBJECT->const_get(env, "Float", true)->as_class() }
        , m_float { static_cast<double>(number) } { }

    static FloatValue *nan(Env *env) {
        auto *value = new FloatValue { env, 0.0 };
        value->m_nan = true;
        return value;
    }

    double to_double() {
        return m_float;
    }

    bool is_zero() {
        return m_float == 0;
    }

    bool is_nan() {
        return m_nan;
    }

private:
    double m_float { 0.0 };
    bool m_nan { false };
};

}
