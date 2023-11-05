#include "natalie.hpp"
#include "string.h"

#include <math.h>

namespace Natalie {

Value f_to_i_or_bigint(double value) {
    assert(value == ::round(value));
    return IntegerObject::create(Integer(value));
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
    if (is_nan())
        return false;
    if (other->is_integer()) {
        auto integer = other->as_integer();
        return integer->integer() == m_double;
    }
    if (other->is_float()) {
        auto *f = other->as_float();
        return f->m_double == m_double;
    }
    return other.send(env, "=="_s, { this })->is_truthy();
}

bool FloatObject::eql(Value other) const {
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
            return Value::floatingpoint(m_double);                               \
                                                                                 \
        FloatObject *result;                                                     \
        if (precision == 0)                                                      \
            return f_to_i_or_bigint(::libm_name(m_double));                      \
                                                                                 \
        double f = ::pow(10, precision);                                         \
        double rounded = ::libm_name(m_double * f) / f;                          \
        if (isinf(f) || isinf(rounded)) {                                        \
            return Value::floatingpoint(m_double);                               \
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

// from dtoa.c by David Gay
extern "C" char *dtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve);

Value FloatObject::to_s() const {
    if (is_nan()) {
        return new StringObject { "NaN", Encoding::US_ASCII };
    } else if (is_positive_infinity()) {
        return new StringObject { "Infinity", Encoding::US_ASCII };
    } else if (is_negative_infinity()) {
        return new StringObject { "-Infinity", Encoding::US_ASCII };
    }

    int decpt, sign;
    char *out, *e;
    out = dtoa(to_double(), 0, 0, &decpt, &sign, &e);

    String string;

    auto add_exp = [decpt](String &s) {
        if (s.length() > 1) {
            s.insert(1, '.');
        } else {
            s.append(".0");
        }
        s.append_sprintf("e%+03d", decpt - 1);
    };

    if (decpt > 0) {
        string = String(out);
        long long s_length = string.length();
        if (decpt < s_length) {
            string.insert(decpt, '.');
        } else if (decpt <= DBL_DIG) {
            if (decpt > s_length) {
                string.append(decpt - s_length, '0');
            }
            string.append(".0");
        } else {
            add_exp(string);
        }
    } else if (decpt > -4) {
        string = String("0.");
        string.append(::abs(decpt), '0');
        string.append(out);
    } else {
        string = String(out);
        add_exp(string);
    }

    if (sign) {
        string.prepend_char('-');
    }

    return new StringObject { string, Encoding::US_ASCII };
}

Value FloatObject::cmp(Env *env, Value rhs) {
    Value lhs = this;

    auto infinite_sym = "infinite?"_s;

    auto call_is_infinite = [&](Value value) -> Value {
        if (value->respond_to(env, infinite_sym))
            return value->send(env, infinite_sym);
        return NilObject::the();
    };

    if (is_positive_infinity()) {
        if (call_is_infinite(rhs) == Value::integer(1))
            return Value::integer(0);
        else
            return Value::integer(1);
    } else if (is_negative_infinity()) {
        if (call_is_infinite(rhs) == Value::integer(-1))
            return Value::integer(0);
        else
            return Value::integer(-1);
    }

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
        break;
    case Object::Type::Integer:
        ary->push(Value::floatingpoint(arg->as_integer()->integer().to_double()));
        break;
    default:
        ary->push(KernelModule::Float(env, arg, true));
    }
    ary->push(this);
    return ary;
}

Value FloatObject::denominator(Env *env) const {
    if (!is_finite()) {
        return Value::integer(1);
    }
    return to_r(env)->as_rational()->denominator(env);
}

Value FloatObject::numerator(Env *env) const {
    if (!is_finite()) {
        return to_f();
    }
    return to_r(env)->as_rational()->numerator(env);
}

Value FloatObject::to_i(Env *env) const {
    if (is_nan()) env->raise("FloatDomainError", "NaN");
    if (is_infinity()) {
        env->raise("FloatDomainError", to_s()->as_string()->string());
    }
    double num = m_double;
    if (num > 0.0) num = ::floor(num);
    if (num < 0.0) num = ::ceil(num);
    return f_to_i_or_bigint(num);
}

Value FloatObject::to_r(Env *env) const {
    return KernelModule::Rational(env, m_double);
}

Value FloatObject::add(Env *env, Value rhs) {
    Value lhs = this;

    if (rhs->is_complex()) {
        return rhs->send(env, "+"_s, { lhs });
    }

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "+"_s, { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double addend1 = to_double();
    double addend2 = rhs->as_float()->to_double();
    return Value::floatingpoint(addend1 + addend2);
}

Value FloatObject::sub(Env *env, Value rhs) {
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
    return Value::floatingpoint(minuend - subtrahend);
}

Value FloatObject::mul(Env *env, Value rhs) {
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
    return Value::floatingpoint(multiplicand * multiplier);
}

Value FloatObject::div(Env *env, Value rhs) {
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

    return Value::floatingpoint(dividend / divisor);
}

Value FloatObject::mod(Env *env, Value rhs) {
    Value lhs = this;

    bool rhs_is_non_zero = (rhs->is_float() && !rhs->as_float()->is_zero()) || (rhs->is_integer() && !rhs->as_integer()->is_zero());

    if (rhs->is_float() && rhs->as_float()->is_negative_infinity()) return Value::floatingpoint(rhs->as_float()->to_double());
    if (is_negative_zero() && rhs_is_non_zero) return Value::floatingpoint(m_double);

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

    auto result = fmod(dividend, divisor);

    if (result != 0.0 && signbit(dividend) != signbit(divisor))
        result += divisor;

    return Value::floatingpoint(result);
}

Value FloatObject::divmod(Env *env, Value arg) {
    if (is_nan()) env->raise("FloatDomainError", "NaN");
    if (is_infinity()) env->raise("FloatDomainError", "Infinity");

    if (!arg->is_numeric()) env->raise("TypeError", "{} can't be coerced into Float", arg->klass()->inspect_str());
    if (arg->is_float() && arg->as_float()->is_nan()) env->raise("FloatDomainError", "NaN");
    if (arg->is_float() && arg->as_float()->is_zero()) env->raise("ZeroDivisionError", "divided by 0");
    if (arg->is_integer() && arg->as_integer()->is_zero()) env->raise("ZeroDivisionError", "divided by 0");

    Value division = div(env, arg);
    Value modulus = mod(env, arg);

    return new ArrayObject {
        f_to_i_or_bigint(::floor(division->as_float()->to_double())),
        modulus
    };
}

Value FloatObject::pow(Env *env, Value rhs) {
    Value lhs = this;

    if ((rhs->is_float() || rhs->is_rational()) && to_double() < 0) {
        auto comp = new ComplexObject { this };
        return comp->send(env, "**"_s, { rhs });
    }

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, "**"_s, { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double base = to_double();
    double exponent = rhs->as_float()->to_double();

    if (base < 0 && ::floor(exponent) != exponent)
        env->raise("ArgumentError", "Not yet implemented: negative raised to a fractional power");

    return Value::floatingpoint(::pow(base, exponent));
}

Value FloatObject::abs(Env *env) const {
    return Value::floatingpoint(fabs(m_double));
}

Value FloatObject::next_float(Env *env) const {
    return Value::floatingpoint(::nextafter(to_double(), HUGE_VAL));
}

Value FloatObject::prev_float(Env *env) const {
    return Value::floatingpoint(::nextafter(to_double(), -HUGE_VAL));
}

Value FloatObject::arg(Env *env) {
    if (is_nan()) return this;
    if (!signbit(m_double)) {
        return Value::integer(0);
    } else {
        return fetch_nested_const({ "Math"_s, "PI"_s });
    }
}

#define NAT_DEFINE_FLOAT_COMPARISON_METHOD(name, op)                                                        \
    bool FloatObject::name(Env *env, Value rhs) {                                                           \
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
            return false;                                                                                   \
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
