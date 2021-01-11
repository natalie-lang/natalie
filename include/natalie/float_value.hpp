#pragma once

#include <assert.h>
#include <float.h>
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
        : Value { Value::Type::Float, env->Object()->const_fetch(env, "Float")->as_class() }
        , m_double { number } { }

    FloatValue(Env *env, nat_int_t number)
        : Value { Value::Type::Float, env->Object()->const_fetch(env, "Float")->as_class() }
        , m_double { static_cast<double>(number) } { }

    FloatValue(const FloatValue &other)
        : Value { Value::Type::Float, const_cast<FloatValue &>(other).klass() }
        , m_double { other.m_double } { }

    static FloatValue *nan(Env *env) {
        return new FloatValue { env, 0.0 / 0.0 };
    }

    static FloatValue *positive_infinity(Env *env) {
        return new FloatValue { env, std::numeric_limits<double>::infinity() };
    }

    static FloatValue *negative_infinity(Env *env) {
        return new FloatValue { env, -std::numeric_limits<double>::infinity() };
    }

    static FloatValue *max(Env *env) {
        return new FloatValue { env, DBL_MAX };
    }

    static FloatValue *neg_max(Env *env) {
        return new FloatValue { env, -DBL_MAX };
    }

    static FloatValue *min(Env *env) {
        return new FloatValue { env, DBL_MIN };
    }

    double to_double() {
        return m_double;
    }

    ValuePtr to_int_no_truncation(Env *env) {
        if (is_nan() || is_infinity()) return this;
        if (m_double == ::floor(m_double)) {
            return new IntegerValue { env, static_cast<nat_int_t>(m_double) };
        }
        return this;
    }

    bool is_zero() {
        return m_double == 0 && !is_nan();
    }

    bool is_finite() {
        return !(is_negative_infinity() || is_positive_infinity() || is_nan());
    }

    bool is_nan() {
        return isnan(m_double);
    }

    bool is_infinity() {
        return isinf(m_double);
    }

    bool is_negative() {
        return m_double < 0.0;
    };

    bool is_positive() {
        return m_double > 0.0;
    };

    // NOTE: even though this is a predicate method with a ? suffix, it returns an 1, -1, or nil.
    ValuePtr is_infinite(Env *);

    bool is_positive_infinity() {
        return is_infinity() && m_double > 0;
    }

    bool is_negative_infinity() {
        return is_infinity() && m_double < 0;
    }

    bool is_positive_zero() {
        return m_double == 0 && !signbit(m_double);
    }

    bool is_negative_zero() {
        return m_double == 0 && signbit(m_double);
    }

    FloatValue *negate() {
        FloatValue *copy = new FloatValue { *this };
        copy->m_double *= -1;
        return copy;
    }

    bool eq(Env *, ValuePtr);

    bool eql(ValuePtr);

    ValuePtr abs(Env *);
    ValuePtr add(Env *, ValuePtr);
    ValuePtr arg(Env *);
    ValuePtr ceil(Env *, ValuePtr);
    ValuePtr cmp(Env *, ValuePtr);
    ValuePtr coerce(Env *, ValuePtr);
    ValuePtr div(Env *, ValuePtr);
    ValuePtr divmod(Env *, ValuePtr);
    ValuePtr floor(Env *, ValuePtr);
    ValuePtr mod(Env *, ValuePtr);
    ValuePtr mul(Env *, ValuePtr);
    ValuePtr next_float(Env *);
    ValuePtr pow(Env *, ValuePtr);
    ValuePtr prev_float(Env *);
    ValuePtr round(Env *, ValuePtr);
    ValuePtr sub(Env *, ValuePtr);
    ValuePtr to_f() { return this; }
    ValuePtr to_i(Env *);
    ValuePtr to_s(Env *);
    ValuePtr truncate(Env *, ValuePtr);

    bool lt(Env *, ValuePtr);
    bool lte(Env *, ValuePtr);
    bool gt(Env *, ValuePtr);
    bool gte(Env *, ValuePtr);

    ValuePtr uminus() {
        return negate();
    }

    ValuePtr uplus() {
        return this;
    }

    static void build_constants(Env *env, ClassValue *klass) {
        klass->const_set(env, "DIG", new FloatValue { env, double { DBL_DIG } });
        klass->const_set(env, "EPSILON", new FloatValue { env, std::numeric_limits<double>::epsilon() });
        klass->const_set(env, "INFINITY", FloatValue::positive_infinity(env));
        klass->const_set(env, "MANT_DIG", new FloatValue { env, double { DBL_MANT_DIG } });
        klass->const_set(env, "MAX", FloatValue::max(env));
        klass->const_set(env, "MAX_10_EXP", new FloatValue { env, double { DBL_MAX_10_EXP } });
        klass->const_set(env, "MAX_EXP", new FloatValue { env, double { DBL_MAX_EXP } });
        klass->const_set(env, "MIN", FloatValue::min(env));
        klass->const_set(env, "MIN_10_EXP", new FloatValue { env, double { DBL_MIN_10_EXP } });
        klass->const_set(env, "MIN_EXP", new FloatValue { env, double { DBL_MIN_EXP } });
        klass->const_set(env, "NAN", FloatValue::nan(env));
        klass->const_set(env, "NAN", FloatValue::nan(env));
        klass->const_set(env, "RADIX", new FloatValue { env, double { std::numeric_limits<double>::radix } });
    }

private:
    double m_double { 0.0 };
};
}
