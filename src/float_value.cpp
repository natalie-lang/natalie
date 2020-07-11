#include "natalie.hpp"
#include "string.h"

#include <math.h>

namespace Natalie {

bool FloatValue::eq(Env *env, Value &other) {
    if (other.is_integer()) {
        return m_float == other.as_integer()->to_int64_t() && !m_nan && !m_infinity;
    }
    if (other.is_float()) {
        auto *f = other.as_float();
        return f->m_float == m_float && f->m_nan == m_nan && f->m_infinity == m_infinity;
    }
    if (other.respond_to(env, "==")) {
        Value *args[] = { this };
        return other.send(env, "==", 1, args)->is_truthy();
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

}
