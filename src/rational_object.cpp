#include "natalie.hpp"
#include "natalie/integer_object.hpp"

namespace Natalie {

RationalObject *RationalObject::create(Env *env, IntegerObject *numerator, IntegerObject *denominator) {
    if (IntegerObject::is_zero(denominator))
        env->raise("ZeroDivisionError", "divided by 0");

    if (IntegerObject::is_negative(denominator)) {
        numerator = IntegerObject::negate(env, numerator)->as_integer();
        denominator = IntegerObject::negate(env, denominator)->as_integer();
    }

    auto gcd = IntegerObject::gcd(env, numerator, denominator);
    numerator = IntegerObject::div(env, numerator, gcd)->as_integer();
    denominator = IntegerObject::div(env, denominator, gcd)->as_integer();

    return new RationalObject { numerator, denominator };
}

Value RationalObject::add(Env *env, Value other) {
    if (other->is_integer()) {
        auto numerator = IntegerObject::add(env, IntegerObject::integer(m_numerator), IntegerObject::mul(env, m_denominator, other))->as_integer();
        return new RationalObject { numerator, m_denominator };
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->add(env, other);
    } else if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env)->as_integer();
        auto den1 = other->as_rational()->denominator(env)->as_integer();
        auto a = IntegerObject::mul(env, den1, m_numerator);
        auto b = IntegerObject::mul(env, m_denominator, num1);
        auto c = IntegerObject::add(env, IntegerObject::integer(a->as_integer()), b)->as_integer();
        auto den2 = IntegerObject::mul(env, den1, m_denominator)->as_integer();
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
        if (IntegerObject::eq(env, IntegerObject::integer(m_denominator), Value::integer(1))) {
            return IntegerObject::cmp(env, IntegerObject::integer(m_numerator), other->as_integer());
        }
        other = new RationalObject { other->as_integer(), new IntegerObject { 1 } };
    }
    if (other->is_rational()) {
        auto rational = other->as_rational();
        auto num1 = IntegerObject::mul(env, m_numerator, rational->denominator(env));
        auto num2 = IntegerObject::mul(env, m_denominator, rational->numerator(env));
        auto a = IntegerObject::sub(env, IntegerObject::integer(num1->as_integer()), num2)->as_integer();
        return IntegerObject::cmp(env, IntegerObject::integer(a), Value::integer(0));
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
        if (IntegerObject::is_zero(complex->imaginary(env)->as_integer())) {
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

        if (IntegerObject::integer(arg->m_denominator) == 0)
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
        return IntegerObject::to_nat_int_t(m_denominator) == 1 && IntegerObject::eq(env, IntegerObject::integer(m_numerator), other);

    if (other->is_float())
        return to_f(env)->as_float()->eq(env, other);

    if (!other->is_rational())
        return other.send(env, "=="_s, { this })->is_truthy();

    if (!IntegerObject::eq(env, IntegerObject::integer(m_numerator), other->as_rational()->m_numerator))
        return false;

    if (!IntegerObject::eq(env, IntegerObject::integer(m_denominator), other->as_rational()->m_denominator))
        return false;

    return true;
}

Value RationalObject::floor(Env *env, Value precision_value) {
    nat_int_t precision = 0;
    if (precision_value)
        precision = IntegerObject::convert_to_nat_int_t(env, precision_value);

    if (IntegerObject::integer(m_denominator) == 1)
        return IntegerObject::floor(env, m_numerator, precision_value);

    if (precision < 0)
        return IntegerObject::floor(env, to_i(env)->as_integer(), precision_value);
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
        auto num2 = IntegerObject::mul(env, m_numerator, num1);
        auto den2 = IntegerObject::mul(env, m_denominator, den1);
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
        if (IntegerObject::is_zero(numerator))
            return create(env, new IntegerObject { 1 }, new IntegerObject { 1 });

        if (IntegerObject::is_zero(m_numerator) && IntegerObject::is_negative(numerator))
            env->raise("ZeroDivisionError", "divided by 0");

        if (IntegerObject::integer(denominator) == 1) {
            Value new_numerator, new_denominator;
            if (IntegerObject::integer(numerator).is_negative()) {
                if (IntegerObject::is_zero(m_numerator))
                    env->raise("ZeroDivisionError", "divided by 0");
                auto negated = IntegerObject::negate(env, numerator);
                new_numerator = IntegerObject::pow(env, IntegerObject::integer(m_denominator), negated);
                new_denominator = IntegerObject::pow(env, IntegerObject::integer(m_numerator), negated);
            } else {
                new_numerator = IntegerObject::pow(env, IntegerObject::integer(m_numerator), numerator);
                new_denominator = IntegerObject::pow(env, IntegerObject::integer(m_denominator), numerator);
            }

            if (new_numerator->is_integer() && new_denominator->is_integer())
                return create(env, new_numerator->as_integer(), new_denominator->as_integer());
        }
    }

    return this->to_f(env)->as_float()->pow(env, other);
}

Value RationalObject::sub(Env *env, Value other) {
    if (other->is_integer()) {
        auto numerator = IntegerObject::sub(env, IntegerObject::integer(m_numerator), IntegerObject::mul(env, m_denominator, other))->as_integer();
        return new RationalObject { numerator, m_denominator };
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->sub(env, other);
    } else if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env)->as_integer();
        auto den1 = other->as_rational()->denominator(env)->as_integer();
        auto a = IntegerObject::mul(env, den1, m_numerator);
        auto b = IntegerObject::mul(env, m_denominator, num1);
        auto c = IntegerObject::sub(env, IntegerObject::integer(a->as_integer()), b)->as_integer();
        auto den2 = IntegerObject::mul(env, den1, m_denominator)->as_integer();
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
    if (IntegerObject::is_negative(m_numerator)) {
        auto a = IntegerObject::negate(env, m_numerator)->as_integer();
        auto b = IntegerObject::div(env, a, m_denominator)->as_integer();
        return IntegerObject::negate(env, b);
    }
    return IntegerObject::div(env, m_numerator, m_denominator);
}

Value RationalObject::to_s(Env *env) {
    return StringObject::format("{}/{}", m_numerator->inspect_str(env), m_denominator->inspect_str(env));
}

Value RationalObject::rationalize(Env *env) {
    return this;
}

Value RationalObject::truncate(Env *env, Value ndigits) {
    auto numerator = IntegerObject::to_nat_int_t(m_numerator);
    auto denominator = IntegerObject::to_nat_int_t(m_denominator);
    nat_int_t digits = 0;

    if (ndigits) {
        if (!ndigits->is_integer())
            env->raise("TypeError", "not an integer");
        digits = IntegerObject::to_nat_int_t(ndigits->as_integer());
    }

    if (digits == 0)
        return IntegerObject::create(numerator / denominator);

    if (digits < 0) {
        auto quotient = IntegerObject(numerator / denominator);
        return IntegerObject::truncate(env, &quotient, ndigits);
    }

    const auto power = static_cast<nat_int_t>(std::pow(10, digits));
    return RationalObject::create(env, new IntegerObject { numerator * power / denominator }, new IntegerObject { power });
}

}
