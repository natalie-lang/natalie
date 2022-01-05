#include "natalie.hpp"
#include "string.h"

#include <math.h>

namespace Natalie {

Value f_to_i_or_bigint(double value) {
    assert(value == ::round(value));
    if (value >= (double)NAT_INT_MAX || value <= (double)NAT_INT_MAX) {
        auto *bignum = new BignumObject { value };
        if (bignum->has_to_be_bignum())
            return bignum;
    }
    return Value::integer(value);
}

Value FloatObject::is_infinite(Env *env) const {
    if (is_positive_infinity()) {
        return Value::integer(1);
    } else if (is_negative_infinity()) {
        return Value::integer(-1);
    } else {
        return NilObject::the();
    }
}

bool FloatObject::eq(Env *env, Value other) {
    if (other.is_fast_integer())
        return m_double == other.get_fast_integer();
    if (other.is_fast_float())
        return m_double == other.get_fast_float();

    if (other->is_integer()) {
        return m_double == other->as_integer()->to_nat_int_t();
    }
    if (other->is_float()) {
        auto *f = other->as_float();
        return f->m_double == m_double;
    }
    auto equal_symbol = "=="_s;
    if (other->respond_to(env, equal_symbol)) {
        return other.send(env, equal_symbol, { this })->is_truthy();
    }
    return false;
}

bool FloatObject::eql(Value other) const {
    if (other.is_fast_float())
        return m_double == other.get_fast_float();
    if (other.is_fast_integer())
        return false;
    other.unguard();

    if (!other->is_float()) return false;
    auto *f = other->as_float();
    return f->m_double == m_double;
}

#define ROUNDING_OPERATION(name, libm_name)                                      \
    Value FloatObject::name(Env *env, Value precision_value) {                   \
        nat_int_t precision = 0;                                                 \
        if (precision_value) {                                                   \
            if (precision_value->is_float()) {                                   \
                precision_value = precision_value->as_float()->to_i(env);        \
            }                                                                    \
            precision_value->assert_type(env, Object::Type::Integer, "Integer"); \
            precision = precision_value->as_integer()->to_nat_int_t();           \
        }                                                                        \
        if (precision <= 0 && (is_nan() || is_infinity()))                       \
            env->raise("FloatDomainError", this->inspect_str(env));              \
                                                                                 \
        if (is_infinity())                                                       \
            return Value { m_double };                                           \
                                                                                 \
        FloatObject *result;                                                     \
        if (precision == 0)                                                      \
            return f_to_i_or_bigint(::libm_name(m_double));                      \
                                                                                 \
        double f = ::pow(10, precision);                                         \
        double rounded = ::libm_name(m_double * f) / f;                          \
        if (isinf(f) || isinf(rounded)) {                                        \
            return Value { m_double };                                           \
        }                                                                        \
        if (precision < 0)                                                       \
            return f_to_i_or_bigint(rounded);                                    \
                                                                                 \
        /* precision > 0 */                                                      \
        return new FloatObject { rounded };                                      \
    }

ROUNDING_OPERATION(floor, floor)
ROUNDING_OPERATION(round, round)
ROUNDING_OPERATION(ceil, ceil)
ROUNDING_OPERATION(truncate, trunc)

Value FloatObject::to_s(Env *env) const {
    if (is_nan()) {
        return new StringObject { "NaN" };
    } else if (is_positive_infinity()) {
        return new StringObject { "Infinity" };
    } else if (is_negative_infinity()) {
        return new StringObject { "-Infinity" };
    }

    int decpt, sign;
    char *out, *e;
    out = dtoa(to_double(), 0, 0, &decpt, &sign, &e);

    String *string;

    auto add_exp = [decpt](String *out) {
        if (out->length() > 1) {
            out->insert(1, '.');
        } else {
            out->append(".0");
        }
        out->append_sprintf("e%+03d", decpt - 1);
    };

    if (decpt > 0) {
        string = new String { out };
        long long s_length = string->length();
        if (decpt < s_length) {
            string->insert(decpt, '.');
        } else if (decpt <= DBL_DIG) {
            if (decpt > s_length) {
                string->append(decpt - s_length, '0');
            }
            string->append(".0");
        } else {
            add_exp(string);
        }
    } else if (decpt > -4) {
        string = new String { "0." };
        string->append(::abs(decpt), '0');
        string->append(out);
    } else {
        string = new String { out };
        add_exp(string);
    }

    if (sign) {
        string->prepend_char('-');
    }

    return new StringObject { string };
}

Value FloatObject::cmp(Env *env, Value rhs) {
    if (rhs.is_fast_float()) {
        double lhs_d = m_double;
        double rhs_d = rhs.get_fast_float();

        if (lhs_d < rhs_d) {
            return Value::integer(-1);
        } else if (lhs_d == rhs_d) {
            return Value::integer(0);
        } else {
            return Value::integer(1);
        }
    }
    if (rhs.is_fast_integer()) {
        double lhs_d = m_double;
        nat_int_t rhs_i = rhs.get_fast_integer();

        if (lhs_d < rhs_i) {
            return Value::integer(-1);
        } else if (lhs_d == rhs_i) {
            return Value::integer(0);
        } else {
            return Value::integer(1);
        }
    }
    rhs.unguard();

    Value lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "<=>"_s, { rhs });
    if (!rhs->is_float()) return NilObject::the();

    if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {
        return NilObject::the();
    }

    double lhs_d = lhs->as_float()->to_double();
    double rhs_d = rhs->as_float()->to_double();

    if (lhs_d < rhs_d) {
        return Value::integer(-1);
    } else if (lhs_d == rhs_d) {
        return Value::integer(0);
    } else {
        return Value::integer(1);
    }
}

Value FloatObject::coerce(Env *env, Value arg) {
    ArrayObject *ary = new ArrayObject {};
    switch (arg->type()) {
    case Object::Type::Float:
        ary->push(arg);
        ary->push(this);
        break;
    case Object::Type::Integer:
        ary->push(new FloatObject { arg->as_integer()->to_nat_int_t() });
        ary->push(this);
        break;
    case Object::Type::String:
        printf("TODO\n");
        abort();
        break;
    default:
        env->raise("ArgumentError", "invalid value for Float(): {}", arg->inspect_str(env));
    }
    return ary;
}

Value FloatObject::to_i(Env *env) const {
    return f_to_i_or_bigint(::floor(m_double));
}

Value FloatObject::add(Env *env, Value rhs) {
    if (rhs.is_fast_integer()) {
        return Value { m_double + rhs.get_fast_integer() };
    }
    if (rhs.is_fast_float()) {
        return Value { m_double + rhs.get_fast_float() };
    }
    rhs.unguard();

    Value lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "+"_s, { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double addend1 = to_double();
    double addend2 = rhs->as_float()->to_double();
    return Value { addend1 + addend2 };
}

Value FloatObject::sub(Env *env, Value rhs) {
    if (rhs.is_fast_integer()) {
        return Value { m_double - rhs.get_fast_integer() };
    }
    if (rhs.is_fast_float()) {
        return Value { m_double - rhs.get_fast_float() };
    }
    rhs.unguard();

    Value lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "-"_s, { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double minuend = to_double();
    double subtrahend = rhs->as_float()->to_double();
    return Value { minuend - subtrahend };
}

Value FloatObject::mul(Env *env, Value rhs) {
    if (rhs.is_fast_integer()) {
        return Value { m_double * rhs.get_fast_integer() };
    }
    if (rhs.is_fast_float()) {
        return Value { m_double * rhs.get_fast_float() };
    }
    rhs.unguard();

    Value lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "*"_s, { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double multiplicand = to_double();
    double multiplier = rhs->as_float()->to_double();
    return Value { multiplicand * multiplier };
}

Value FloatObject::div(Env *env, Value rhs) {
    if (rhs.is_fast_integer()) {
        return Value { m_double / rhs.get_fast_integer() };
    }
    if (rhs.is_fast_float()) {
        return Value { m_double / rhs.get_fast_float() };
    }
    rhs.unguard();

    Value lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "/"_s, { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double dividend = to_double();
    double divisor = rhs->as_float()->to_double();

    return Value { dividend / divisor };
}

Value FloatObject::mod(Env *env, Value rhs) {
    if (rhs.is_fast_integer()) {
        if (rhs.get_fast_integer() == 0) env->raise("ZeroDivisionError", "divided by 0");
        return Value { fmod(m_double, rhs.get_fast_integer()) };
    }
    if (rhs.is_fast_float()) {
        if (rhs.get_fast_float() == 0) env->raise("ZeroDivisionError", "divided by 0");
        if (rhs.get_fast_float() == -INFINITY) return rhs;
        return Value { fmod(m_double, rhs.get_fast_float()) };
    }
    rhs.unguard();

    Value lhs = this;

    bool rhs_is_non_zero = (rhs->is_float() && !rhs->as_float()->is_zero()) || (rhs->is_integer() && !rhs->as_integer()->is_zero());

    if (rhs->is_float() && rhs->as_float()->is_negative_infinity()) return rhs;
    if (is_negative_zero() && rhs_is_non_zero) return this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "%"_s, { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double dividend = to_double();
    double divisor = rhs->as_float()->to_double();

    if (divisor == 0) env->raise("ZeroDivisionError", "divided by 0");

    return Value { fmod(dividend, divisor) };
}

Value FloatObject::divmod(Env *env, Value arg) {
    if (is_nan()) env->raise("FloatDomainError", "NaN");
    if (is_infinity()) env->raise("FloatDomainError", "Infinity");

    if (arg.is_fast_integer()) {
        if (arg.get_fast_integer() == 0) env->raise("ZeroDivisionError", "divided by 0");
        return new ArrayObject {
            Value { f_to_i_or_bigint(::floor(m_double / arg.get_fast_integer())) },
            Value { ::fmod(m_double, arg.get_fast_integer()) }
        };
    }
    if (arg.is_fast_float()) {
        if (arg.get_fast_float() == 0) env->raise("ZeroDivisionError", "divided by 0");
        if (isnan(arg.get_fast_float())) env->raise("FloatDomainError", "NaN");
        return new ArrayObject {
            Value { f_to_i_or_bigint(::floor(m_double / arg.get_fast_float())) },
            Value { arg.get_fast_float() == -INFINITY ? ::fmod(m_double, arg.get_fast_integer()) : -INFINITY }
        };
    }
    arg.unguard();

    if (!arg->is_numeric()) env->raise("TypeError", "{} can't be coerced into Float", arg->klass()->inspect_str());
    if (arg->is_float() && arg->as_float()->is_nan()) env->raise("FloatDomainError", "NaN");
    if (arg->is_float() && arg->as_float()->is_zero()) env->raise("ZeroDivisionError", "divided by 0");
    if (arg->is_integer() && arg->as_integer()->is_zero()) env->raise("ZeroDivisionError", "divided by 0");

    Value division = div(env, arg);
    Value modulus = mod(env, arg);

    return new ArrayObject {
        f_to_i_or_bigint(::floor(division.get_fast_float())),
        modulus
    };
}

Value FloatObject::pow(Env *env, Value rhs) {
    if (rhs.is_fast_integer()) {
        return Value { ::pow(m_double, rhs.get_fast_integer()) };
    }
    if (rhs.is_fast_float()) {
        return Value { ::pow(m_double, rhs.get_fast_float()) };
    }
    rhs.unguard();

    Value lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "**"_s, { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double base = to_double();
    double exponent = rhs->as_float()->to_double();

    return Value { ::pow(base, exponent) };
}

Value FloatObject::abs(Env *env) const {
    return Value { fabs(m_double) };
}

Value FloatObject::next_float(Env *env) const {
    return Value { ::nextafter(to_double(), HUGE_VAL) };
}

Value FloatObject::prev_float(Env *env) const {
    return Value { ::nextafter(to_double(), -HUGE_VAL) };
}

Value FloatObject::arg(Env *env) {
    if (is_nan()) return this;
    if (!signbit(m_double)) {
        return Value::integer(0);
    } else {
        return find_nested_const(env, { "Math"_s, "PI"_s });
    }
}

bool FloatObject::optimized_method(SymbolObject *method_name) {
    if (s_optimized_methods.is_empty()) {
        // NOTE: No method that ever returns 'this' can be an "optimized" method. Trust me!
        // FIXME: Is this list correct?
        //        Can it be expanded?
        s_optimized_methods.set("+"_s);
        s_optimized_methods.set("-"_s);
        s_optimized_methods.set("*"_s);
        s_optimized_methods.set("/"_s);
        s_optimized_methods.set("%"_s);
        s_optimized_methods.set("**"_s);
        s_optimized_methods.set("<=>"_s);
        s_optimized_methods.set("=="_s);
        s_optimized_methods.set("==="_s);
        s_optimized_methods.set("<"_s);
        s_optimized_methods.set("<="_s);
        s_optimized_methods.set(">"_s);
        s_optimized_methods.set(">="_s);
        s_optimized_methods.set("eql?"_s);
    }
    return !!s_optimized_methods.get(method_name);
}

#define NAT_DEFINE_FLOAT_COMPARISON_METHOD(name, op)                                                        \
    bool FloatObject::name(Env *env, Value rhs) {                                                           \
        if (rhs.is_fast_float())                                                                            \
            return m_double op rhs.get_fast_float();                                                        \
        if (rhs.is_fast_integer())                                                                          \
            return m_double op rhs.get_fast_integer();                                                      \
        rhs.unguard();                                                                                      \
                                                                                                            \
        Value lhs = this;                                                                                   \
                                                                                                            \
        if (!rhs->is_float()) {                                                                             \
            auto coerced = Natalie::coerce(env, rhs, lhs);                                                  \
            lhs = coerced.first;                                                                            \
            rhs = coerced.second;                                                                           \
        }                                                                                                   \
                                                                                                            \
        if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern(NAT_QUOTE(op)), { rhs });           \
        if (!rhs->is_float()) {                                                                             \
            env->raise("ArgumentError", "comparison of Float with {} failed", rhs->klass()->inspect_str()); \
        }                                                                                                   \
                                                                                                            \
        if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {                                       \
            return NilObject::the();                                                                        \
        }                                                                                                   \
                                                                                                            \
        double lhs_d = lhs->as_float()->to_double();                                                        \
        double rhs_d = rhs->as_float()->to_double();                                                        \
                                                                                                            \
        return lhs_d op rhs_d;                                                                              \
    }

NAT_DEFINE_FLOAT_COMPARISON_METHOD(lt, <)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(lte, <=)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(gt, >)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(gte, >=)
}
