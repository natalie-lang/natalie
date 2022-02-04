#include "natalie.hpp"

namespace Natalie {

Value RationalObject::coerce(Env *env, Value other) {
    if (other->is_integer()) {
        return new ArrayObject { new RationalObject(other->as_integer(), new IntegerObject { 1 }), this };
    } else if (other->is_float()) {
        return new ArrayObject { other, this->to_f(env) };
    } else if (other->is_rational()) {
        return new ArrayObject { other, this };
    }

    env->raise("TypeError", "{} can't be coerced into {}", other->klass()->inspect_str(), this->klass()->inspect_str());
}

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
    return StringObject::format("({}/{})", m_numerator->inspect_str(env), m_denominator->inspect_str(env));
}

Value RationalObject::numerator(Env *env) {
    return m_numerator;
}

Value RationalObject::to_f(Env *env) {
    return m_numerator->send(env, "fdiv"_s, { m_denominator });
}

Value RationalObject::to_i(Env *env) {
    if (m_numerator->is_negative()) {
        return m_numerator->negate(env)->as_integer()->div(env, m_denominator)->as_integer()->negate(env);
    }
    return m_numerator->div(env, m_denominator);
}

Value RationalObject::to_s(Env *env) {
    return StringObject::format("{}/{}", m_numerator->inspect_str(env), m_denominator->inspect_str(env));
}

}
