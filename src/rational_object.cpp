#include "natalie.hpp"
#include "natalie/integer_object.hpp"

namespace Natalie {

RationalObject *RationalObject::create(Env *env, Integer numerator, Integer denominator) {
    if (IntegerObject::is_zero(denominator))
        env->raise("ZeroDivisionError", "divided by 0");

    if (denominator.is_negative()) {
        numerator = -numerator;
        denominator = -denominator;
    }

    auto gcd = IntegerObject::gcd(env, numerator, denominator).integer();
    numerator = numerator / gcd;
    denominator = denominator / gcd;

    return new RationalObject { numerator, denominator };
}

Value RationalObject::add(Env *env, Value other) {
    if (other.is_integer()) {
        auto numerator = IntegerObject::add(env, m_numerator, IntegerObject::mul(env, m_denominator, other))->as_integer();
        return new RationalObject { numerator, m_denominator };
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->add(env, other);
    } else if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env).integer();
        auto den1 = other->as_rational()->denominator(env).integer();
        auto a = den1 * m_numerator;
        auto b = num1 * m_denominator;
        auto c = a + b;
        auto den2 = den1 * m_denominator;
        return create(env, c, den2);
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this);
        return result.first->send(env, "+"_s, { result.second });
    } else {
        env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
    }
}

Value RationalObject::cmp(Env *env, Value other) {
    if (other.is_integer()) {
        if (m_denominator == 1)
            return IntegerObject::cmp(env, m_numerator, other->as_integer());
        other = new RationalObject { other.integer(), Value::integer(1) };
    }
    if (other->is_rational()) {
        auto rational = other->as_rational();
        auto num1 = m_numerator * rational->denominator(env).integer();
        auto num2 = m_denominator * rational->numerator(env).integer();
        auto a = num1 - num2;
        return IntegerObject::cmp(env, a, Value::integer(0));
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
    if (other.is_integer()) {
        return new ArrayObject { new RationalObject(other.integer(), Value::integer(1)), this };
    } else if (other->is_float()) {
        return new ArrayObject { other, this->to_f(env) };
    } else if (other->is_rational()) {
        return new ArrayObject { other, this };
    } else if (other->is_complex()) {
        auto complex = other->as_complex();
        if (complex->imaginary(env).integer().is_zero()) {
            auto a = new RationalObject { complex->real(env), Value::integer(1) };
            auto b = new ComplexObject(this);
            return new ArrayObject { a, b };
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
    if (other.is_integer() || other->is_rational()) {
        RationalObject *arg;
        if (other.is_integer()) {
            arg = create(env, Integer(1), other.integer());
        } else {
            auto numerator = other->as_rational()->numerator(env).integer();
            auto denominator = other->as_rational()->denominator(env).integer();
            arg = create(env, denominator, numerator);
        }

        if (arg->m_denominator.is_zero())
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
    if (other.is_integer())
        return m_denominator == 1 && m_numerator == other.integer();

    if (other->is_float())
        return to_f(env)->as_float()->eq(env, other);

    if (!other->is_rational())
        return other.send(env, "=="_s, { this })->is_truthy();

    if (m_numerator != other->as_rational()->m_numerator)
        return false;

    if (m_denominator != other->as_rational()->m_denominator)
        return false;

    return true;
}

Value RationalObject::floor(Env *env, Value precision_value) {
    nat_int_t precision = 0;
    if (precision_value)
        precision = IntegerObject::convert_to_nat_int_t(env, precision_value);

    if (m_denominator == 1)
        return IntegerObject::floor(env, m_numerator, precision_value);

    if (precision < 0)
        return IntegerObject::floor(env, to_i(env)->as_integer(), precision_value);
    if (precision == 0)
        return to_f(env)->as_float()->floor(env, precision_value);

    auto powered = Natalie::pow(10, precision);
    auto numerator = mul(env, powered)->as_rational()->floor(env, nullptr).integer();

    return create(env, numerator, powered);
}

Value RationalObject::inspect(Env *env) {
    return StringObject::format("({}/{})", m_numerator, m_denominator);
}

Value RationalObject::marshal_dump(Env *env) {
    return new ArrayObject { m_numerator, m_denominator };
}

Value RationalObject::mul(Env *env, Value other) {
    if (other.is_integer())
        other = new RationalObject { other.integer(), Value::integer(1) };

    if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env).integer();
        auto den1 = other->as_rational()->denominator(env).integer();
        auto num2 = m_numerator * num1;
        auto den2 = m_denominator * den1;
        return create(env, num2, den2);
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
    Integer numerator, denominator;

    if (other.is_integer()) {
        numerator = other.integer();
        denominator = 1;
    } else if (other->is_rational()) {
        numerator = other->as_rational()->numerator(env).integer();
        denominator = other->as_rational()->denominator(env).integer();
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->pow(env, other);
    } else {
        if (other->respond_to(env, "coerce"_s)) {
            auto result = Natalie::coerce(env, other, this);
            return result.first->send(env, "**"_s, { result.second });
        } else {
            env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
        }
    }

    if (numerator.is_zero())
        return create(env, Integer(1), Integer(1));

    if (m_numerator.is_zero() && numerator.is_negative())
        env->raise("ZeroDivisionError", "divided by 0");

    if (denominator == 1) {
        Value new_numerator, new_denominator;
        if (numerator.is_negative()) {
            if (m_numerator.is_zero())
                env->raise("ZeroDivisionError", "divided by 0");
            auto negated = -numerator;
            new_numerator = IntegerObject::pow(env, m_denominator, negated);
            new_denominator = IntegerObject::pow(env, m_numerator, negated);
        } else {
            new_numerator = IntegerObject::pow(env, m_numerator, numerator);
            new_denominator = IntegerObject::pow(env, m_denominator, numerator);
        }

        if (new_numerator.is_integer() && new_denominator.is_integer())
            return create(env, new_numerator.integer(), new_denominator.integer());
    }

    return this->to_f(env)->as_float()->pow(env, other);
}

Value RationalObject::sub(Env *env, Value other) {
    if (other.is_integer()) {
        auto numerator = m_numerator - m_denominator * other.integer();
        return new RationalObject { numerator, m_denominator };
    } else if (other->is_float()) {
        return this->to_f(env)->as_float()->sub(env, other);
    } else if (other->is_rational()) {
        auto num1 = other->as_rational()->numerator(env).integer();
        auto den1 = other->as_rational()->denominator(env).integer();
        auto a = den1 * m_numerator;
        auto b = m_denominator * num1;
        auto c = a - b;
        auto den2 = den1 * m_denominator;
        return create(env, c, den2);
    } else if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this);
        return result.first->send(env, "-"_s, { result.second });
    } else {
        env->raise("TypeError", "{} can't be coerced into Rational", other->klass()->inspect_str());
    }
}

Value RationalObject::to_f(Env *env) {
    return Value(m_numerator)->send(env, "fdiv"_s, { m_denominator });
}

Value RationalObject::to_i(Env *env) {
    if (IntegerObject::is_negative(m_numerator)) {
        auto a = -m_numerator;
        auto b = a / m_denominator;
        return -b;
    }
    return IntegerObject::div(env, m_numerator, m_denominator);
}

Value RationalObject::to_s(Env *env) {
    return StringObject::format("{}/{}", m_numerator, m_denominator);
}

Value RationalObject::rationalize(Env *env) {
    return this;
}

Value RationalObject::truncate(Env *env, Value ndigits) {
    auto numerator = m_numerator.to_nat_int_t();
    auto denominator = m_denominator.to_nat_int_t();
    nat_int_t digits = 0;

    if (ndigits) {
        if (!ndigits.is_integer())
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
    return RationalObject::create(env, numerator * power / denominator, power);
}

}
