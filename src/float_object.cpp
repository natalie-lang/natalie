#include "natalie.hpp"
#include "string.h"

#include <math.h>

namespace Natalie {

ValuePtr FloatObject::is_infinite(Env *env) const {
    if (is_positive_infinity()) {
        return ValuePtr::integer(1);
    } else if (is_negative_infinity()) {
        return ValuePtr::integer(-1);
    } else {
        return NilObject::the();
    }
}

bool FloatObject::eq(Env *env, ValuePtr other) {
    if (other->is_integer()) {
        return m_double == other->as_integer()->to_nat_int_t();
    }
    if (other->is_float()) {
        auto *f = other->as_float();
        return f->m_double == m_double;
    }
    auto equal_symbol = SymbolObject::intern("==");
    if (other->respond_to(env, equal_symbol)) {
        return other.send(env, equal_symbol, { this })->is_truthy();
    }
    return false;
}

bool FloatObject::eql(ValuePtr other) const {
    if (!other->is_float()) return false;
    auto *f = other->as_float();
    return f->m_double == m_double;
}

ValuePtr FloatObject::ceil(Env *env, ValuePtr precision_value) const {
    double value = this->to_double();
    nat_int_t precision = 0;
    if (precision_value) {
        precision_value->assert_type(env, Object::Type::Integer, "Integer");
        precision = precision_value->as_integer()->to_nat_int_t();
    }
    FloatObject *result;
    if (precision == 0) {
        result = new FloatObject { ::ceil(value) };
    } else {
        double f = ::pow(10, precision);
        value = ::ceil(value * f) / f;
        result = new FloatObject { value };
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

ValuePtr FloatObject::floor(Env *env, ValuePtr precision_value) const {
    double value = this->to_double();
    nat_int_t precision = 0;
    if (precision_value) {
        precision_value->assert_type(env, Object::Type::Integer, "Integer");
        precision = precision_value->as_integer()->to_nat_int_t();
    }
    FloatObject *result;
    if (precision == 0) {
        result = new FloatObject { ::floor(value) };
    } else {
        double f = ::pow(10, precision);
        value = ::floor(value * f) / f;
        result = new FloatObject { value };
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

ValuePtr FloatObject::round(Env *env, ValuePtr precision_value) {
    double value = this->to_double();
    nat_int_t precision = 0;
    if (precision_value) {
        if (precision_value->is_float()) {
            precision_value = precision_value->as_float()->to_i(env);
        }
        precision_value->assert_type(env, Object::Type::Integer, "Integer");
        precision = precision_value->as_integer()->to_nat_int_t();
    }
    if (precision <= 0 && (is_nan() || is_infinity())) {
        env->raise("FloatDomainError", this->inspect_str(env));
    }
    if (is_infinity()) {
        return new FloatObject { value };
    }
    FloatObject *result;
    if (precision == 0) {
        result = new FloatObject { ::round(value) };
    } else {
        double f = ::pow(10, precision);
        double rounded = ::round(value * f) / f;
        if (isinf(f) || isinf(rounded)) {
            result = new FloatObject { value };
        } else {
            result = new FloatObject { rounded };
        }
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

ValuePtr FloatObject::truncate(Env *env, ValuePtr precision_value) const {
    if (is_negative()) {
        return ceil(env, precision_value);
    }
    return floor(env, precision_value);
}

ValuePtr FloatObject::to_s(Env *env) const {
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

ValuePtr FloatObject::cmp(Env *env, ValuePtr other) {
    ValuePtr lhs = this;
    ValuePtr rhs = other;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern("<=>"), { rhs });
    if (!rhs->is_float()) return NilObject::the();

    if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {
        return NilObject::the();
    }

    double lhs_d = lhs->as_float()->to_double();
    double rhs_d = rhs->as_float()->to_double();

    if (lhs_d < rhs_d) {
        return ValuePtr::integer(-1);
    } else if (lhs_d == rhs_d) {
        return ValuePtr::integer(0);
    } else {
        return ValuePtr::integer(1);
    }
}

ValuePtr FloatObject::coerce(Env *env, ValuePtr arg) {
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

ValuePtr FloatObject::to_i(Env *env) const {
    return floor(env, nullptr);
}

ValuePtr FloatObject::add(Env *env, ValuePtr rhs) {
    ValuePtr lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern("+"), { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double addend1 = to_double();
    double addend2 = rhs->as_float()->to_double();
    return new FloatObject { addend1 + addend2 };
}

ValuePtr FloatObject::sub(Env *env, ValuePtr rhs) {
    ValuePtr lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern("-"), { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double minuend = to_double();
    double subtrahend = rhs->as_float()->to_double();
    return new FloatObject { minuend - subtrahend };
}

ValuePtr FloatObject::mul(Env *env, ValuePtr rhs) {
    ValuePtr lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern("*"), { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double multiplicand = to_double();
    double mulitiplier = rhs->as_float()->to_double();
    return new FloatObject { multiplicand * mulitiplier };
}

ValuePtr FloatObject::div(Env *env, ValuePtr rhs) {
    ValuePtr lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern("/"), { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double dividend = to_double();
    double divisor = rhs->as_float()->to_double();

    return new FloatObject { dividend / divisor };
}

ValuePtr FloatObject::mod(Env *env, ValuePtr rhs) {
    ValuePtr lhs = this;

    bool rhs_is_non_zero = (rhs->is_float() && !rhs->as_float()->is_zero()) || (rhs->is_integer() && !rhs->as_integer()->is_zero());

    if (rhs->is_float() && rhs->as_float()->is_negative_infinity()) return rhs;
    if (is_negative_zero() && rhs_is_non_zero) return this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern("%"), { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double dividend = to_double();
    double divisor = rhs->as_float()->to_double();

    if (divisor == 0.0) env->raise("ZeroDivisionError", "divided by 0");

    return new FloatObject { fmod(dividend, divisor) };
}

ValuePtr FloatObject::divmod(Env *env, ValuePtr arg) {
    if (is_nan()) env->raise("FloatDomainError", "NaN");
    if (is_infinity()) env->raise("FloatDomainError", "Infinity");

    if (!arg->is_numeric()) env->raise("TypeError", "{} can't be coerced into Float", arg->klass()->class_name_or_blank());
    if (arg->is_float() && arg->as_float()->is_nan()) env->raise("FloatDomainError", "NaN");
    if (arg->is_float() && arg->as_float()->is_zero()) env->raise("ZeroDivisionError", "divided by 0");
    if (arg->is_integer() && arg->as_integer()->is_zero()) env->raise("ZeroDivisionError", "divided by 0");

    ValuePtr division = div(env, arg);
    ValuePtr modulus = mod(env, arg);

    ArrayObject *ary = new ArrayObject {};
    ary->push(division->as_float()->floor(env, 0));
    ary->push(modulus);

    return ary;
}

ValuePtr FloatObject::pow(Env *env, ValuePtr rhs) {
    ValuePtr lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern("**"), { rhs });
    if (!rhs->is_float()) rhs->assert_type(env, Object::Type::Float, "Float");

    double base = to_double();
    double exponent = rhs->as_float()->to_double();

    return new FloatObject { ::pow(base, exponent) };
}

ValuePtr FloatObject::abs(Env *env) const {
    auto number = to_double();
    if (number == -0.0 || number < 0.0) {
        return negate();
    } else {
        return new FloatObject { *this };
    }
}

ValuePtr FloatObject::next_float(Env *env) const {
    double number = nextafter(to_double(), HUGE_VAL);
    return new FloatObject { number };
}

ValuePtr FloatObject::prev_float(Env *env) const {
    double number = nextafter(to_double(), -HUGE_VAL);
    return new FloatObject { number };
}

ValuePtr FloatObject::arg(Env *env) {
    if (is_nan()) return this;
    if (!signbit(m_double)) {
        return ValuePtr::integer(0);
    } else {
        ValuePtr Math = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("Math"));
        return Math->const_fetch(SymbolObject::intern("PI"));
    }
}

#define NAT_DEFINE_FLOAT_COMPARISON_METHOD(name, op)                                                                \
    bool FloatObject::name(Env *env, ValuePtr rhs) {                                                                \
        ValuePtr lhs = this;                                                                                        \
                                                                                                                    \
        if (!rhs->is_float()) {                                                                                     \
            auto coerced = Natalie::coerce(env, rhs, lhs);                                                          \
            lhs = coerced.first;                                                                                    \
            rhs = coerced.second;                                                                                   \
        }                                                                                                           \
                                                                                                                    \
        if (!lhs->is_float()) return lhs.send(env, SymbolObject::intern(NAT_QUOTE(op)), { rhs });                   \
        if (!rhs->is_float()) {                                                                                     \
            env->raise("ArgumentError", "comparison of Float with {} failed", rhs->klass()->class_name_or_blank()); \
        }                                                                                                           \
                                                                                                                    \
        if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {                                               \
            return NilObject::the();                                                                                \
        }                                                                                                           \
                                                                                                                    \
        double lhs_d = lhs->as_float()->to_double();                                                                \
        double rhs_d = rhs->as_float()->to_double();                                                                \
                                                                                                                    \
        return lhs_d op rhs_d;                                                                                      \
    }

NAT_DEFINE_FLOAT_COMPARISON_METHOD(lt, <)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(lte, <=)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(gt, >)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(gte, >=)

}
