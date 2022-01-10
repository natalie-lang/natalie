#include "natalie.hpp"
#include "natalie/constants.hpp"

namespace Natalie {

Value BignumObject::to_f() const {
    return new FloatObject { m_bigint->to_double() };
}

Value BignumObject::to_s(Env *env, Value base_value) {
    if (base_value) {
        // NATFIXME
        NAT_UNREACHABLE();
    }
    return new StringObject { m_bigint->to_string().c_str() };
}

Value BignumObject::abs(Env *env) {
    if (m_bigint->is_negative()) {
        return new BignumObject { -to_bigint() };
    } else {
        return this;
    }
}

Value BignumObject::add(Env *env, Value arg) {
    if (arg->is_float()) {
        auto result = m_bigint->to_double() + arg->as_float()->to_double();
        return new FloatObject { result };
    }

    if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    return new BignumObject { to_bigint() + other->to_bigint() };
}

Value BignumObject::sub(Env *env, Value arg) {
    if (arg->is_float()) {
        auto result = m_bigint->to_double() - arg->as_float()->to_double();
        return new FloatObject { result };
    }

    if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    return new BignumObject { to_bigint() - other->to_bigint() };
}

Value BignumObject::mul(Env *env, Value arg) {
    if (arg->is_float()) {
        auto result = m_bigint->to_double() * arg->as_float()->to_double();
        return new FloatObject { result };
    }

    if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    return new BignumObject { to_bigint() * other->to_bigint() };
}

Value BignumObject::div(Env *env, Value arg) {
    if (arg->is_float()) {
        auto result = m_bigint->to_double() / arg->as_float()->to_double();
        return new FloatObject { result };
    }

    if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    if (other->is_fixnum() && arg->as_integer()->to_nat_int_t() == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    return new BignumObject { to_bigint() / other->to_bigint() };
}

Value BignumObject::mod(Env *env, Value arg) {
    if (arg->is_float())
        arg = Value::integer(arg->as_float()->to_double());

    if (!arg->is_integer())
        arg = Natalie::coerce(env, arg, this).second;

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    if (other->is_fixnum() && arg->as_integer()->to_nat_int_t() == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    return new BignumObject { to_bigint() % other->to_bigint() };
}

Value BignumObject::pow(Env *env, Value arg) {
    nat_int_t nat_int;
    if (arg.is_fast_integer()) {
        nat_int = arg.get_fast_integer();
    } else {
        arg.unguard();

        if (arg->is_float())
            return FloatObject { m_bigint->to_double() }.pow(env, arg);

        if (!arg->is_integer()) {
            auto coerced = Natalie::coerce(env, arg, this);
            arg = coerced.second;
            if (!coerced.first->is_integer()) {
                return coerced.first->send(env, "**"_s, { arg });
            }
        }

        arg->assert_type(env, Object::Type::Integer, "Integer");
        if (arg->as_integer()->is_bignum()) {
            env->warn("in a**b, b may be too big");
            return FloatObject { m_bigint->to_double() }.pow(env, arg);
        }

        nat_int = arg->as_integer()->to_nat_int_t();
    }

    if (to_bigint() == 0 && nat_int < 0)
        env->raise("ZeroDivisionError", "divided by 0");

    // NATFIXME: If a negative number is passed we want to return a Rational
    if (nat_int < 0)
        NAT_NOT_YET_IMPLEMENTED();

    // NATFIXME: Ruby has a really weird max bignum value that is calculated by the words needed to store a bignum
    // The calculation that we do is pretty much guessed to be in the direction of ruby. However, we should do more research about this limit...
    size_t length = m_bigint->to_string().length();
    constexpr const size_t BIGINT_LIMIT = 8 * 1024 * 1024;
    if (length > BIGINT_LIMIT || length * nat_int > BIGINT_LIMIT) {
        env->warn("in a**b, b may be too big");
        return FloatObject { m_bigint->to_double() }.pow(env, arg);
    }

    return new BignumObject { ::pow(to_bigint(), nat_int) };
}

Value BignumObject::negate(Env *env) {
    return new BignumObject { -to_bigint() };
}

Value BignumObject::times(Env *env, Block *block) {
    if (!block) {
        auto enumerator = send(env, "enum_for"_s, { "times"_s });
        enumerator->ivar_set(env, "@size"_s, *m_bigint < 0 ? Value::integer(0) : this);
        return enumerator;
    }

    if (*m_bigint <= 0)
        return this;

    // Performance: We could yield fixnums for the first NAT_MAX_FIXNUM numbers
    for (BigInt i = 0; i <= *m_bigint; i++) {
        Value num = new BignumObject { i };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &num, nullptr);
    }

    return this;
}

Value BignumObject::gcd(Env *env, Value divisor) {
    divisor->assert_type(env, Object::Type::Integer, "Integer");
    auto other = divisor->as_integer();
    return new BignumObject { ::gcd(to_bigint(), other->to_bigint()) };
}

bool BignumObject::eq(Env *env, Value other) {
    if (other->is_float()) {
        return to_bigint() == other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bigint() == other->as_integer()->to_bigint();
    }
    return other->send(env, "=="_s, { this })->is_truthy();
}

bool BignumObject::lt(Env *env, Value other) {
    if (other->is_float()) {
        return to_bigint() < other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bigint() < other->as_integer()->to_bigint();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool BignumObject::lte(Env *env, Value other) {
    if (other->is_float()) {
        return to_bigint() <= other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bigint() <= other->as_integer()->to_bigint();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool BignumObject::gt(Env *env, Value other) {
    if (other->is_float()) {
        return to_bigint() > other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bigint() > other->as_integer()->to_bigint();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool BignumObject::gte(Env *env, Value other) {
    if (other->is_float()) {
        return to_bigint() >= other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bigint() >= other->as_integer()->to_bigint();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}
}
