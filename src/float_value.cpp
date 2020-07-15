#include "natalie.hpp"
#include "string.h"

#include <math.h>

namespace Natalie {

bool FloatValue::eq(Env &env, Value &other) {
    if (other.is_integer()) {
        return m_float == other.as_integer()->to_int64_t() && !m_nan && !m_infinity;
    }
    if (other.is_float()) {
        auto *f = other.as_float();
        return f->m_float == m_float && f->m_nan == m_nan && f->m_infinity == m_infinity;
    }
    if (other.respond_to(&env, "==")) {
        Value *args[] = { this };
        return other.send(&env, "==", 1, args)->is_truthy();
    }
    return false;
}

bool FloatValue::eql(Value &other) {
    if (!other.is_float()) return false;
    auto *f = other.as_float();
    return f->m_float == m_float && f->m_nan == m_nan && f->m_infinity == m_infinity;
}

Value *FloatValue::ceil(Env *env, int64_t precision) {
    double value = this->to_double();
    FloatValue *result;
    if (precision == 0) {
        result = new FloatValue { env, ::ceil(value) };
    } else {
        value *= pow(10, precision);
        result = new FloatValue { env, ::ceil(value) * (pow(10, (-1 * precision))) };
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

Value *FloatValue::floor(Env *env, int64_t precision) {
    double value = this->to_double();
    FloatValue *result;
    if (precision == 0) {
        result = new FloatValue { env, ::floor(value) };
    } else {
        value *= pow(10, precision);
        result = new FloatValue { env, ::floor(value) * (pow(10, (-1 * precision))) };
    }
    if (precision <= 0) {
        return result->to_int_no_truncation(env);
    }
    return result;
}

Value *FloatValue::to_s(Env &env) {
    if (is_nan()) {
        return new StringValue { &env, "NaN" };
    } else if (is_positive_infinity()) {
        return new StringValue { &env, "Infinity" };
    } else if (is_negative_infinity()) {
        return new StringValue { &env, "-Infinity" };
    }

    char out[100]; // probably overkill
    snprintf(out, 100, "%.15f", to_double());
    int len = strlen(out);
    while (len > 1 && out[len - 1] == '0' && out[len - 2] != '.') {
        out[--len] = '\0';
    }

    return new StringValue { &env, out };
}

Value *FloatValue::cmp(Env &env, Value &other) {
    Value *lhs = this;
    Value *rhs = &other;

    if (!rhs->is_float()) {
        auto coerced = coerce(&env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(&env, "<=>", 1, &rhs);
    if (!rhs->is_float()) return env.nil();

    if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {
        return env.nil();
    }

    double lhs_d = lhs->as_float()->to_double();
    double rhs_d = rhs->as_float()->to_double();

    if (lhs_d < rhs_d) {
        return new IntegerValue { &env, -1 };
    } else if (lhs_d == rhs_d) {
        return new IntegerValue { &env, 0 };
    } else {
        return new IntegerValue { &env, 1 };
    }
}

}
