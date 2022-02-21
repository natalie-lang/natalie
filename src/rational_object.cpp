#include "natalie.hpp"

namespace Natalie {

Value RationalObject::add(Env *env, Value other) {
    if (other->is_integer()) {
        auto numerator = m_numerator->add(env, m_denominator->mul(env, other))->as_integer();
        return new RationalObject { numerator, m_denominator };
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->add(env, other);
    } else if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env)->as_integer();
        auto den1 = other->as_rational()->denominator(env)->as_integer();
        auto gcd1 = m_denominator->gcd(env, den1);
        auto a = den1->div(env, gcd1)->as_integer()->mul(env, m_numerator);
        auto b = m_denominator->div(env, gcd1)->as_integer()->mul(env, num1);
        auto c = a->as_integer()->add(env, b)->as_integer();
        auto gcd2 = c->gcd(env, gcd1);
        auto num2 = c->div(env, gcd2);
        auto den2 = den1->div(env, gcd2)->as_integer()->mul(env, m_denominator->div(env, gcd1));
        return new RationalObject { num2->as_integer(), den2->as_integer() };
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, "+"_s, { result.second });
    } else {
        env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
    }
}

Value RationalObject::cmp(Env *env, Value other) {
    if (other->is_integer()) {
        if (m_denominator->eq(env, Value::integer(1))) {
            return m_numerator->cmp(env, other->as_integer());
        }
        other = new RationalObject { other->as_integer(), new IntegerObject { 1 } };
    }
    if (other->is_rational()) {
        auto rational = other->as_rational();
        auto num1 = m_numerator->mul(env, rational->denominator(env));
        auto num2 = m_denominator->mul(env, rational->numerator(env));
        return num1->as_integer()->sub(env, num2)->as_integer()->cmp(env, Value::integer(0));
    }
    if (other->is_float()) {
        return to_f(env)->as_float()->cmp(env, other->as_float());
    }
    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, "<=>"_s, { result.second });
    }
    return NilObject::the();
}

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

Value RationalObject::mul(Env *env, Value other) {
    if (other->is_integer()) {
        other = new RationalObject { other->as_integer(), new IntegerObject { 1 } };
    }
    if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env)->as_integer();
        auto den1 = other->as_rational()->denominator(env)->as_integer();
        auto gcd1 = m_numerator->gcd(env, den1);
        auto gcd2 = m_denominator->gcd(env, num1);
        auto num2 = m_numerator->div(env, gcd1)->as_integer()->mul(env, num1->div(env, gcd2));
        auto den2 = m_denominator->div(env, gcd2)->as_integer()->mul(env, den1->div(env, gcd1));
        return new RationalObject { num2->as_integer(), den2->as_integer() };
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->mul(env, other);
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, "*"_s, { result.second });
    } else {
        env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
    }
}

Value RationalObject::numerator(Env *env) {
    return m_numerator;
}

Value RationalObject::sub(Env *env, Value other) {
    if (other->is_integer()) {
        auto numerator = m_numerator->sub(env, m_denominator->mul(env, other))->as_integer();
        return new RationalObject { numerator, m_denominator };
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->sub(env, other);
    } else if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env)->as_integer();
        auto den1 = other->as_rational()->denominator(env)->as_integer();
        auto gcd1 = m_denominator->gcd(env, den1);
        auto a = den1->div(env, gcd1)->as_integer()->mul(env, m_numerator);
        auto b = m_denominator->div(env, gcd1)->as_integer()->mul(env, num1);
        auto c = a->as_integer()->sub(env, b)->as_integer();
        auto gcd2 = c->gcd(env, gcd1);
        auto num2 = c->div(env, gcd2);
        auto den2 = den1->div(env, gcd2)->as_integer()->mul(env, m_denominator->div(env, gcd1));
        return new RationalObject { num2->as_integer(), den2->as_integer() };
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, "-"_s, { result.second });
    } else {
        env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
    }
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
