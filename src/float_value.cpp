#include "natalie.hpp"
#include "string.h"

#include <math.h>

namespace Natalie {

Value *FloatValue::is_infinite(Env *env) {
    if (is_positive_infinity()) {
        return new IntegerValue { env, 1 };
    } else if (is_negative_infinity()) {
        return new IntegerValue { env, -1 };
    } else {
        return NAT_NIL;
    }
}

bool FloatValue::eq(Env *env, Value *other) {
    if (other->is_integer()) {
        return m_float == other->as_integer()->to_int64_t() && !m_nan && !m_infinity;
    }
    if (other->is_float()) {
        auto *f = other->as_float();
        return f->m_float == m_float && f->m_nan == m_nan && f->m_infinity == m_infinity;
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
    return f->m_float == m_float && f->m_nan == m_nan && f->m_infinity == m_infinity;
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
        value *= ::pow(10, precision);
        result = new FloatValue { env, ::ceil(value) * (::pow(10, (-1 * precision))) };
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
        value *= ::pow(10, precision);
        result = new FloatValue { env, ::floor(value) * (::pow(10, (-1 * precision))) };
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

Value *FloatValue::to_s(Env *env) {
    if (is_nan()) {
        return new StringValue { env, "NaN" };
    } else if (is_positive_infinity()) {
        return new StringValue { env, "Infinity" };
    } else if (is_negative_infinity()) {
        return new StringValue { env, "-Infinity" };
    }

    char out[100]; // probably overkill
    snprintf(out, 100, "%.15f", to_double());
    int len = strlen(out);
    while (len > 1 && out[len - 1] == '0' && out[len - 2] != '.') {
        out[--len] = '\0';
    }

    return new StringValue { env, out };
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
    if (!rhs->is_float()) return env->nil();

    if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {
        return env->nil();
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
    switch (arg->type) {
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

    if (rhs->is_float()) {
        if (rhs->as_float()->is_nan()) return FloatValue::nan(env);
        if (rhs->as_float()->is_positive_infinity()) return new FloatValue { env, 0.0 };
        if (rhs->as_float()->is_negative_infinity()) return new FloatValue { env, -0.0 };
    }

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "/", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double dividend = to_double();
    double divisor = rhs->as_float()->to_double();

    if (divisor == 0.0) {
        if (dividend < 0.0) {
            return FloatValue::negative_infinity(env);
        } else if (dividend > 0.0) {
            return FloatValue::positive_infinity(env);
        } else {
            return FloatValue::nan(env);
        }
    }
    return new FloatValue { env, dividend / divisor };
}

Value *FloatValue::mod(Env *env, Value *rhs) {
    Value *lhs = this;

    if (rhs->is_float() && rhs->as_float()->is_nan()) return FloatValue::nan(env);
    if (rhs->is_float() && rhs->as_float()->is_positive_infinity()) return new FloatValue { env, 0.0 };
    if (rhs->is_float() && rhs->as_float()->is_negative_infinity()) return new FloatValue { env, -0.0 };

    if (!rhs->is_float()) {
        auto coerced = Natalie::coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "%", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double dividend = to_double();
    double divisor = rhs->as_float()->to_double();

    if (divisor == 0.0) {
        if (dividend < 0.0) {
            return FloatValue::negative_infinity(env);
        } else if (dividend > 0.0) {
            return FloatValue::positive_infinity(env);
        } else {
            return FloatValue::nan(env);
        }
    }
    return new FloatValue { env, fmod(dividend, divisor) };
}

Value *FloatValue::divmod(Env *env, Value *arg) {
    if (is_nan()) NAT_RAISE(env, "FloatDomainError", "NaN");
    if (is_infinity()) NAT_RAISE(env, "FloatDomainError", "Infinity");

    if (!arg->is_numeric()) NAT_RAISE(env, "TypeError", "%s can't be coerced into Float", arg->klass->class_name());
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
    double x, y;
    x = as_float()->to_double();
    y = nextafterf(x, DBL_MAX);
    return new FloatValue { env, y };
}

Value *FloatValue::prev_float(Env *env) {
    double x, y;
    x = as_float()->to_double();
    y = nextafterf(x, DBL_MIN);
    return new FloatValue { env, y };
}

#define NAT_DEFINE_FLOAT_COMPARISON_METHOD(name, op)                                                         \
    bool FloatValue::name(Env *env, Value *rhs) {                                                            \
        Value *lhs = this;                                                                                   \
                                                                                                             \
        if (!rhs->is_float()) {                                                                              \
            auto coerced = Natalie::coerce(env, rhs, lhs);                                                   \
            lhs = coerced.first;                                                                             \
            rhs = coerced.second;                                                                            \
        }                                                                                                    \
                                                                                                             \
        if (!lhs->is_float()) return lhs->send(env, NAT_QUOTE(op), 1, &rhs);                                 \
        if (!rhs->is_float()) {                                                                              \
            NAT_RAISE(env, "ArgumentError", "comparison of Float with %s failed", rhs->klass->class_name()); \
        }                                                                                                    \
                                                                                                             \
        if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {                                        \
            return NAT_NIL;                                                                                  \
        }                                                                                                    \
                                                                                                             \
        double lhs_d = lhs->as_float()->to_double();                                                         \
        double rhs_d = rhs->as_float()->to_double();                                                         \
                                                                                                             \
        return lhs_d op rhs_d;                                                                               \
    }

NAT_DEFINE_FLOAT_COMPARISON_METHOD(lt, <)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(lte, <=)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(gt, >)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(gte, >=)

}
