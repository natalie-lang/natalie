#pragma once

#include <assert.h>
#include <math.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/integer_value.hpp"
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

    FloatValue(const FloatValue &other)
        : Value { Value::Type::Float, other.klass }
        , m_float { other.m_float }
        , m_nan { other.m_nan }
        , m_infinity { other.m_infinity } {
    }

    static FloatValue *nan(Env *env) {
        auto *value = new FloatValue { env, 0.0 };
        value->m_nan = true;
        return value;
    }

    static FloatValue *positive_infinity(Env *env) {
        auto *value = new FloatValue { env, 1.0 };
        value->m_infinity = true;
        return value;
    }

    static FloatValue *negative_infinity(Env *env) {
        auto *value = new FloatValue { env, -1.0 };
        value->m_infinity = true;
        return value;
    }

    double to_double() {
        return m_float;
    }

    Value *to_int_no_truncation(Env *env) {
        if (m_nan || m_infinity) return this;
        if (m_float == ::floor(m_float)) {
            return new IntegerValue { env, static_cast<int64_t>(m_float) };
        }
        return this;
    }

    bool is_zero() {
        return m_float == 0 && !m_nan;
    }

    bool is_finite() {
        return !(is_negative_infinity() || is_positive_infinity() || is_nan());
    }

    bool is_nan() {
        return m_nan;
    }

    bool is_infinity() {
        return m_infinity;
    }

    bool is_positive_infinity() {
        return m_infinity && m_float > 0;
    }

    bool is_negative_infinity() {
        return m_infinity && m_float < 0;
    }

    FloatValue *negate() {
        FloatValue *copy = new FloatValue { *this };
        copy->m_float *= -1;
        return copy;
    }

    bool eq(Env &, Value &);

    bool eql(Value &);

    Value *ceil(Env *, int64_t);
    Value *floor(Env *, int64_t);

private:
    double m_float { 0.0 };
    bool m_nan { false };
    bool m_infinity { false };
};

}
