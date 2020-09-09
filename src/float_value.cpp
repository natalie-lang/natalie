#include "natalie.hpp"
#include "string.h"

#include <math.h>

extern "C" {
#include "gdtoa.h"
}

namespace Natalie {

Value *FloatValue::is_infinite(Env *env) {
    if (is_positive_infinity()) {
        return new IntegerValue { env, 1 };
    } else if (is_negative_infinity()) {
        return new IntegerValue { env, -1 };
    } else {
        return env->nil_obj();
    }
}

bool FloatValue::eq(Env *env, Value *other) {
    if (other->is_integer()) {
        return m_float == other->as_integer()->to_int64_t();
    }
    if (other->is_float()) {
        auto *f = other->as_float();
        return f->m_float == m_float;
    }
    if (other->respond_to(env, "==")) {
        Value *args[] = { this };
        return other->send(env, "==", 1, args)->is_truthy();
    }
    return false;
}

bool FloatValue::eql(Value *other) {
    if (!other->is_float()) return false;
    auto *f = other->as_float();
    return f->m_float == m_float;
}

Value *FloatValue::ceil(Env *env, Value *precision_value) {
    double value = this->to_double();
    int64_t precision = 0;
    if (precision_value) {
        NAT_ASSERT_TYPE(precision_value, Value::Type::Integer, "Integer");
        precision = precision_value->as_integer()->to_int64_t();
    }
    FloatValue *result;
    if (precision == 0) {
        result = new FloatValue { env, ::ceil(value) };
    } else {
        double f = ::pow(10, precision);
        value = ::ceil(value * f) / f;
        result = new FloatValue { env, value };
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

Value *FloatValue::floor(Env *env, Value *precision_value) {
    double value = this->to_double();
    int64_t precision = 0;
    if (precision_value) {
        NAT_ASSERT_TYPE(precision_value, Value::Type::Integer, "Integer");
        precision = precision_value->as_integer()->to_int64_t();
    }
    FloatValue *result;
    if (precision == 0) {
        result = new FloatValue { env, ::floor(value) };
    } else {
        double f = ::pow(10, precision);
        value = ::floor(value * f) / f;
        result = new FloatValue { env, value };
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

Value *FloatValue::round(Env *env, Value *precision_value) {
    double value = this->to_double();
    int64_t precision = 0;
    if (precision_value) {
        if (precision_value->is_float()) {
            precision_value = precision_value->as_float()->to_i(env);
        }
        NAT_ASSERT_TYPE(precision_value, Value::Type::Integer, "Integer");
        precision = precision_value->as_integer()->to_int64_t();
    }
    if (precision <= 0 && (is_nan() || is_infinity())) {
        NAT_RAISE(env, "FloatDomainError", NAT_INSPECT(this));
    }
    if (is_infinity()) {
        return new FloatValue { env, value };
    }
    FloatValue *result;
    if (precision == 0) {
        result = new FloatValue { env, ::round(value) };
    } else {
        double f = ::pow(10, precision);
        double rounded = ::round(value * f) / f;
        if (isinf(f) || isinf(rounded)) {
            result = new FloatValue { env, value };
        } else {
            result = new FloatValue { env, rounded };
        }
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

Value *FloatValue::truncate(Env *env, Value *precision_value) {
    if (is_negative()) {
        return ceil(env, precision_value);
    }
    return floor(env, precision_value);
}

Value *FloatValue::to_s(Env *env) {
    if (is_nan()) {
        return new StringValue { env, "NaN" };
    } else if (is_positive_infinity()) {
        return new StringValue { env, "Infinity" };
    } else if (is_negative_infinity()) {
        return new StringValue { env, "-Infinity" };
    }

    int decpt, sign;
    char *out, *e;
    out = dtoa(to_double(), 0, 0, &decpt, &sign, &e);

    StringValue *string;

    if (decpt == 0) {
        string = new StringValue { env, "0." };
        string->append(env, out);

    } else if (decpt < 0) {
        string = new StringValue { env, "0." };
        char *zeros = zero_string(::abs(decpt));
        string->append(env, zeros);
        string->append(env, out);

    } else {
        string = new StringValue { env, out };
        if (decpt == string->length()) {
            string->append(env, ".0");
        } else if (decpt > string->length()) {
            char *zeros = zero_string(decpt - string->length());
            string->append(env, zeros);
            string->append(env, ".0");
        } else {
            string->insert(env, decpt, '.');
        }
    }

    if (sign) {
        string->prepend_char(env, '-');
    }

    return string;
}

Value *FloatValue::cmp(Env *env, Value *other) {
    Value *lhs = this;
    Value *rhs = other;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "<=>", 1, &rhs);
    if (!rhs->is_float()) return env->nil_obj();

    if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {
        return env->nil_obj();
    }

    double lhs_d = lhs->as_float()->to_double();
    double rhs_d = rhs->as_float()->to_double();

    if (lhs_d < rhs_d) {
        return new IntegerValue { env, -1 };
    } else if (lhs_d == rhs_d) {
        return new IntegerValue { env, 0 };
    } else {
        return new IntegerValue { env, 1 };
    }
}

Value *FloatValue::coerce(Env *env, Value *arg) {
    ArrayValue *ary = new ArrayValue { env };
    switch (arg->type()) {
    case Value::Type::Float:
        ary->push(arg);
        ary->push(this);
        break;
    case Value::Type::Integer:
        ary->push(new FloatValue { env, arg->as_integer()->to_int64_t() });
        ary->push(this);
        break;
    case Value::Type::String:
        printf("TODO\n");
        abort();
        break;
    default:
        NAT_RAISE(env, "ArgumentError", "invalid value for Float(): %s", NAT_INSPECT(arg));
    }
    return ary;
}

Value *FloatValue::to_i(Env *env) {
    return new IntegerValue { env, static_cast<int64_t>(::floor(to_double())) };
}

Value *FloatValue::add(Env *env, Value *rhs) {
    Value *lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "+", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double addend1 = to_double();
    double addend2 = rhs->as_float()->to_double();
    return new FloatValue { env, addend1 + addend2 };
}

Value *FloatValue::sub(Env *env, Value *rhs) {
    Value *lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "-", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double minuend = to_double();
    double subtrahend = rhs->as_float()->to_double();
    return new FloatValue { env, minuend - subtrahend };
}

Value *FloatValue::mul(Env *env, Value *rhs) {
    Value *lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "*", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double multiplicand = to_double();
    double mulitiplier = rhs->as_float()->to_double();
    return new FloatValue { env, multiplicand * mulitiplier };
}

Value *FloatValue::div(Env *env, Value *rhs) {
    Value *lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "/", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double dividend = to_double();
    double divisor = rhs->as_float()->to_double();

    return new FloatValue { env, dividend / divisor };
}

Value *FloatValue::mod(Env *env, Value *rhs) {
    Value *lhs = this;

    bool rhs_is_non_zero = (rhs->is_float() && !rhs->as_float()->is_zero()) || (rhs->is_integer() && !rhs->as_integer()->is_zero());

    if (rhs->is_float() && rhs->as_float()->is_negative_infinity()) return rhs;
    if (is_negative_zero() && rhs_is_non_zero) return this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "%", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double dividend = to_double();
    double divisor = rhs->as_float()->to_double();

    if (divisor == 0.0) NAT_RAISE(env, "ZeroDivisionError", "divided by 0");

    return new FloatValue { env, fmod(dividend, divisor) };
}

Value *FloatValue::divmod(Env *env, Value *arg) {
    if (is_nan()) NAT_RAISE(env, "FloatDomainError", "NaN");
    if (is_infinity()) NAT_RAISE(env, "FloatDomainError", "Infinity");

    if (!arg->is_numeric()) NAT_RAISE(env, "TypeError", "%s can't be coerced into Float", arg->klass()->class_name());
    if (arg->is_float() && arg->as_float()->is_nan()) NAT_RAISE(env, "FloatDomainError", "NaN");
    if (arg->is_float() && arg->as_float()->is_zero()) NAT_RAISE(env, "ZeroDivisionError", "divided by 0");
    if (arg->is_integer() && arg->as_integer()->is_zero()) NAT_RAISE(env, "ZeroDivisionError", "divided by 0");

    Value *division = div(env, arg);
    Value *modulus = mod(env, arg);

    ArrayValue *ary = new ArrayValue { env };
    ary->push(division->as_float()->floor(env, 0));
    ary->push(modulus);

    return ary;
}

Value *FloatValue::pow(Env *env, Value *rhs) {
    Value *lhs = this;

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "**", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double base = to_double();
    double exponent = rhs->as_float()->to_double();

    return new FloatValue { env, ::pow(base, exponent) };
}

Value *FloatValue::abs(Env *env) {
    auto number = to_double();
    if (number == -0.0 || number < 0.0) {
        return negate();
    } else {
        return this;
    }
}

Value *FloatValue::next_float(Env *env) {
    double number = nextafter(to_double(), HUGE_VAL);
    return new FloatValue { env, number };
}

Value *FloatValue::prev_float(Env *env) {
    double number = nextafter(to_double(), -HUGE_VAL);
    return new FloatValue { env, number };
}

Value *FloatValue::arg(Env *env) {
    if (is_nan()) return this;
    if (!signbit(m_float)) {
        return new IntegerValue { env, 0 };
    } else {
        Value *Math = env->Object()->const_fetch("Math");
        return Math->const_fetch("PI");
    }
}

#define NAT_DEFINE_FLOAT_COMPARISON_METHOD(name, op)                                                           \
    bool FloatValue::name(Env *env, Value *rhs) {                                                              \
        Value *lhs = this;                                                                                     \
                                                                                                               \
        if (!rhs->is_float()) {                                                                                \
            auto coerced = Natalie::coerce(env, rhs, lhs);                                                     \
            lhs = coerced.first;                                                                               \
            rhs = coerced.second;                                                                              \
        }                                                                                                      \
                                                                                                               \
        if (!lhs->is_float()) return lhs->send(env, NAT_QUOTE(op), 1, &rhs);                                   \
        if (!rhs->is_float()) {                                                                                \
            NAT_RAISE(env, "ArgumentError", "comparison of Float with %s failed", rhs->klass()->class_name()); \
        }                                                                                                      \
                                                                                                               \
        if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {                                          \
            return env->nil_obj();                                                                             \
        }                                                                                                      \
                                                                                                               \
        double lhs_d = lhs->as_float()->to_double();                                                           \
        double rhs_d = rhs->as_float()->to_double();                                                           \
                                                                                                               \
        return lhs_d op rhs_d;                                                                                 \
    }

NAT_DEFINE_FLOAT_COMPARISON_METHOD(lt, <)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(lte, <=)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(gt, >)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(gte, >=)

}
