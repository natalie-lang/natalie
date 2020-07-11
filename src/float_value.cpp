#include "natalie.hpp"
#include "string.h"

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

}
