#include "natalie.hpp"

namespace Natalie {

Value RationalObject::denominator(Env *env) {
    return m_denominator;
}

bool RationalObject::eq(Env *env, Value other) {
    if (m_klass != other->klass()) {
        return false;
    }
    if (!m_numerator->eq(env, other->as_rational()->m_numerator)) {
        return false;
    }
    if (!m_denominator->eq(env, other->as_rational()->m_denominator)) {
        return false;
    }
    return true;
}

Value RationalObject::inspect(Env *env) {
    return StringObject::format(env, "({}/{})", m_numerator->inspect_str(env), m_denominator->inspect_str(env));
}

Value RationalObject::numerator(Env *env) {
    return m_numerator;
}

Value RationalObject::to_s(Env *env) {
    return StringObject::format(env, "{}/{}", m_numerator->inspect_str(env), m_denominator->inspect_str(env));
}

}
