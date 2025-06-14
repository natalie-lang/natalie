#pragma once

#include <assert.h>
#include <float.h>
#include <math.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class FloatObject : public Object {
public:
    static FloatObject *nan() {
        if (!s_nan) {
            std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
            s_nan = new FloatObject { 0.0 / 0.0 };
        }
        return s_nan;
    }
    static FloatObject *create(double number) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new FloatObject { number };
    }

    static FloatObject *create(nat_int_t number) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new FloatObject { number };
    }

    static FloatObject *create(const FloatObject &other) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new FloatObject { other };
    }

    static FloatObject *positive_infinity(Env *env) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new FloatObject { std::numeric_limits<double>::infinity() };
    }

    static FloatObject *negative_infinity(Env *env) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new FloatObject { -std::numeric_limits<double>::infinity() };
    }

    static FloatObject *max(Env *env) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new FloatObject { DBL_MAX };
    }

    static FloatObject *neg_max(Env *env) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new FloatObject { -DBL_MAX };
    }

    static FloatObject *min(Env *env) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new FloatObject { DBL_MIN };
    }

    double to_double() const {
        return m_double;
    }

    bool is_zero() const {
        return m_double == 0 && !is_nan();
    }

    bool is_finite() const {
        return !(is_infinity() || is_nan());
    }

    bool is_nan() const {
        return isnan(m_double);
    }

    bool is_infinity() const {
        return isinf(m_double);
    }

    bool is_negative() const {
        return m_double < 0.0;
    };

    bool is_positive() const {
        return m_double > 0.0;
    };

    // NOTE: even though this is a predicate method with a ? suffix, it returns an 1, -1, or nil.
    Value is_infinite(Env *) const;

    bool is_positive_infinity() const {
        return is_infinity() && m_double > 0;
    }

    bool is_negative_infinity() const {
        return is_infinity() && m_double < 0;
    }

    bool is_positive_zero() const {
        return m_double == 0 && !signbit(m_double);
    }

    bool is_negative_zero() const {
        return m_double == 0 && signbit(m_double);
    }

    FloatObject *negate() const {
        FloatObject *copy = new FloatObject { *this };
        copy->m_double *= -1;
        return copy;
    }

    bool eq(Env *, Value);

    bool eql(Value) const;

    Value abs(Env *) const;
    Value add(Env *, Value);
    Value arg(Env *);
    Value ceil(Env *, Optional<Value>);
    Value cmp(Env *, Value);
    Value coerce(Env *, Value);
    Value denominator(Env *) const;
    Value div(Env *, Value);
    Value divmod(Env *, Value);
    Value floor(Env *, Optional<Value>);
    Value mod(Env *, Value);
    Value mul(Env *, Value);
    Value numerator(Env *) const;
    Value next_float(Env *) const;
    Value pow(Env *, Value);
    Value prev_float(Env *) const;
    Value round(Env *, Optional<Value>);
    Value sub(Env *, Value);
    Value to_f() const { return new FloatObject { *this }; }
    Value to_i(Env *) const;
    Value to_r(Env *) const;
    Value to_s() const;
    Value truncate(Env *, Optional<Value>);

    bool lt(Env *, Value);
    bool lte(Env *, Value);
    bool gt(Env *, Value);
    bool gte(Env *, Value);

    Value uminus() const {
        return FloatObject::create(-m_double);
    }

    Value uplus() const {
        return FloatObject::create(m_double);
    }

    static void build_constants(Env *env, ClassObject *klass) {
        klass->const_set("DIG"_s, FloatObject::create(double { DBL_DIG }));
        klass->const_set("EPSILON"_s, FloatObject::create(std::numeric_limits<double>::epsilon()));
        klass->const_set("INFINITY"_s, FloatObject::positive_infinity(env));
        klass->const_set("MANT_DIG"_s, FloatObject::create(double { DBL_MANT_DIG }));
        klass->const_set("MAX"_s, FloatObject::max(env));
        klass->const_set("MAX_10_EXP"_s, FloatObject::create(double { DBL_MAX_10_EXP }));
        klass->const_set("MAX_EXP"_s, FloatObject::create(double { DBL_MAX_EXP }));
        klass->const_set("MIN"_s, FloatObject::min(env));
        klass->const_set("MIN_10_EXP"_s, FloatObject::create(double { DBL_MIN_10_EXP }));
        klass->const_set("MIN_EXP"_s, FloatObject::create(double { DBL_MIN_EXP }));
        klass->const_set("NAN"_s, FloatObject::nan());
        klass->const_set("RADIX"_s, FloatObject::create(double { std::numeric_limits<double>::radix }));
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<FloatObject {h} float={}>", this, m_double);
    }

private:
    FloatObject(double number)
        : Object { Object::Type::Float, GlobalEnv::the()->Float() }
        , m_double { number } { }

    FloatObject(nat_int_t number)
        : Object { Object::Type::Float, GlobalEnv::the()->Float() }
        , m_double { static_cast<double>(number) } { }

    FloatObject(const FloatObject &other)
        : Object { Object::Type::Float, other.klass() }
        , m_double { other.m_double } { }

    inline static FloatObject *s_nan { nullptr };

    double m_double { 0.0 };
};
}
