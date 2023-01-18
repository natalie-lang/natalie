#include "natalie.hpp"

namespace Natalie {

RationalObject *RationalObject::create(Env *env, IntegerObject *numerator, IntegerObject *denominator) {
    if (denominator->is_zero())
        env->raise("ZeroDivisionError", "divided by 0");

    if (denominator->is_negative()) {
        numerator = numerator->negate(env)->as_integer();
        denominator = denominator->negate(env)->as_integer();
    }

    auto gcd = numerator->gcd(env, denominator);
    numerator = numerator->div(env, gcd)->as_integer();
    denominator = denominator->div(env, gcd)->as_integer();

    return new RationalObject { numerator, denominator };
}

Value RationalObject::add(Env *env, Value other) {
    if (other->is_integer()) {
        auto numerator = m_numerator->add(env, m_denominator->mul(env, other))->as_integer();
        return new RationalObject { numerator, m_denominator };
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->add(env, other);
    } else if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env)->as_integer();
        auto den1 = other->as_rational()->denominator(env)->as_integer();
        auto a = den1->mul(env, m_numerator);
        auto b = m_denominator->mul(env, num1);
        auto c = a->as_integer()->add(env, b)->as_integer();
        auto den2 = den1->mul(env, m_denominator)->as_integer();
        return create(env, c, den2);
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this);
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
        auto result = Natalie::coerce(env, other, this);
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
    } else if (other->is_complex()) {
        auto complex = other->as_complex();
        if (complex->imaginary(env)->as_integer()->is_zero()) {
            return new ArrayObject { new RationalObject { complex->real(env)->as_integer(), new IntegerObject { 1 } }, new ComplexObject(this) };
        } else {
            return new ArrayObject { other, new ComplexObject(this) };
        }
    }

    env->raise("TypeError", "{} can't be coerced into {}", other->klass()->inspect_str(), this->klass()->inspect_str());
}

Value RationalObject::denominator(Env *env) {
    return m_denominator;
}

Value RationalObject::div(Env *env, Value other) {
    if (other->is_integer() || other->is_rational()) {
        RationalObject *arg;
        if (other->is_integer()) {
            arg = create(env, new IntegerObject { 1 }, other->as_integer());
        } else {
            auto numerator = other->as_rational()->numerator(env)->as_integer();
            auto denominator = other->as_rational()->denominator(env)->as_integer();
            arg = create(env, denominator, numerator);
        }

        if (arg->m_denominator->integer() == 0)
            env->raise("ZeroDivisionError", "divided by 0");

        return mul(env, arg);
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->div(env, other);
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this);
        return result.first->send(env, "/"_s, { result.second });
    } else {
        env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
    }
}

bool RationalObject::eq(Env *env, Value other) {
    if (other->is_integer())
        return m_denominator->to_nat_int_t() == 1 && m_numerator->eq(env, other);

    if (other->is_float())
        return to_f(env)->as_float()->eq(env, other);

    if (!other->is_rational())
        return other.send(env, "=="_s, { this })->is_truthy();

    if (!m_numerator->eq(env, other->as_rational()->m_numerator))
        return false;

    if (!m_denominator->eq(env, other->as_rational()->m_denominator))
        return false;

    return true;
}

Value RationalObject::floor(Env *env, Value precision_value) {
    nat_int_t precision = 0;
    if (precision_value)
        precision = IntegerObject::convert_to_nat_int_t(env, precision_value);

    if (m_denominator->integer() == 1)
        return m_numerator->floor(env, precision_value);

    if (precision < 0)
        return to_i(env)->as_integer()->floor(env, precision_value);
    if (precision == 0)
        return to_f(env)->as_float()->floor(env, precision_value);

    auto powered = IntegerObject::create(Natalie::pow(10, precision));
    auto numerator = mul(env, powered)->as_rational()->floor(env, nullptr)->as_integer();

    return create(env, numerator, powered->as_integer());
}

Value RationalObject::inspect(Env *env) {
    return StringObject::format("({}/{})", m_numerator->inspect_str(env), m_denominator->inspect_str(env));
}

Value RationalObject::marshal_dump(Env *env) {
    return new ArrayObject { m_numerator, m_denominator };
}

Value RationalObject::mul(Env *env, Value other) {
    if (other->is_integer()) {
        other = new RationalObject { other->as_integer(), new IntegerObject { 1 } };
    }
    if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env);
        auto den1 = other->as_rational()->denominator(env);
        auto num2 = m_numerator->mul(env, num1);
        auto den2 = m_denominator->mul(env, den1);
        return create(env, num2->as_integer(), den2->as_integer());
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->mul(env, other);
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this);
        return result.first->send(env, "*"_s, { result.second });
    } else {
        env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
    }
}

Value RationalObject::numerator(Env *env) {
    return m_numerator;
}

Value RationalObject::pow(Env *env, Value other) {
    IntegerObject *numerator = nullptr, *denominator = nullptr;
    if (other->is_integer()) {
        numerator = other->as_integer();
        denominator = new IntegerObject { 1 };
    } else if (other->is_rational()) {
        numerator = other->as_rational()->numerator(env)->as_integer();
        denominator = other->as_rational()->denominator(env)->as_integer();
    } else if (!other->is_float()) {
        if (other->respond_to(env, "coerce"_s)) {
            auto result = Natalie::coerce(env, other, this);
            return result.first->send(env, "**"_s, { result.second });
        } else {
            env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
        }
    }

    if (numerator && denominator) {
        if (numerator->integer() == 0)
            return create(env, new IntegerObject { 1 }, new IntegerObject { 1 });

        if (m_numerator->integer() == 0 && numerator->is_negative())
            env->raise("ZeroDivisionError", "divided by 0");

        if (denominator->integer() == 1) {
            Value new_numerator, new_denominator;
            if (numerator->integer().is_negative()) {
                if (m_numerator->integer() == 0)
                    env->raise("ZeroDivisionError", "divided by 0");
                auto negated = numerator->negate(env);
                new_numerator = m_denominator->pow(env, negated);
                new_denominator = m_numerator->pow(env, negated);
            } else {
                new_numerator = m_numerator->pow(env, numerator);
                new_denominator = m_denominator->pow(env, numerator);
            }

            if (new_numerator->is_integer() && new_denominator->is_integer())
                return create(env, new_numerator->as_integer(), new_denominator->as_integer());
        }
    }

    return this->to_f(env)->as_float()->pow(env, other);
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
        auto a = den1->mul(env, m_numerator);
        auto b = m_denominator->mul(env, num1);
        auto c = a->as_integer()->sub(env, b)->as_integer();
        auto den2 = den1->mul(env, m_denominator)->as_integer();
        return create(env, c, den2);
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this);
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

Value RationalObject::rationalize(Env *env) {
    return this;
}

Value RationalObject::truncate(Env *env, Value ndigits) {
    auto numerator = m_numerator->to_nat_int_t();
    auto denominator = m_denominator->to_nat_int_t();
    nat_int_t digits = 0;

    if (ndigits) {
        if (!ndigits->is_integer())
            env->raise("TypeError", "not an integer");
        digits = ndigits->as_integer()->to_nat_int_t();
    }

    if (digits == 0)
        return IntegerObject::create(numerator / denominator);

    if (digits < 0)
        return IntegerObject(numerator / denominator).truncate(env, ndigits);

    const auto power = static_cast<nat_int_t>(std::pow(10, digits));
    return RationalObject::create(env, new IntegerObject { numerator * power / denominator }, new IntegerObject { power });
}

}
