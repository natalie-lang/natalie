#include "natalie.hpp"

namespace Natalie {

ValuePtr BignumObject::to_s(Env *env, ValuePtr base_value) {
    if (base_value) {
        // NATFIXME
        NAT_UNREACHABLE();
    }
    return new StringObject { m_bignum->to_string().c_str() };
}

ValuePtr BignumObject::add(Env *env, ValuePtr arg) {
    if (arg->is_float()) {
        double current = strtod(m_bignum->to_string().c_str(), NULL);
        auto result = current + arg->as_float()->to_double();
        return new FloatObject { result };
    }

    if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    return new BignumObject { to_bignum() + other->to_bignum() };
}

ValuePtr BignumObject::sub(Env *env, ValuePtr arg) {
    if (arg->is_float()) {
        double current = strtod(m_bignum->to_string().c_str(), NULL);
        auto result = current - arg->as_float()->to_double();
        return new FloatObject { result };
    }

    if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    return new BignumObject { to_bignum() - other->to_bignum() };
}

ValuePtr BignumObject::mul(Env *env, ValuePtr arg) {
    if (arg->is_float()) {
        double current = strtod(m_bignum->to_string().c_str(), NULL);
        auto result = current * arg->as_float()->to_double();
        return new FloatObject { result };
    }

    if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    return new BignumObject { to_bignum() * other->to_bignum() };
}

ValuePtr BignumObject::div(Env *env, ValuePtr arg) {
    if (arg->is_float()) {
        double current = strtod(m_bignum->to_string().c_str(), NULL);
        auto result = current / arg->as_float()->to_double();
        return new FloatObject { result };
    }

    if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    if (other->is_fixnum() && arg->as_integer()->to_nat_int_t() == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    return new BignumObject { to_bignum() / other->to_bignum() };
}

ValuePtr BignumObject::negate(Env *env) {
    return new BignumObject { -to_bignum() };
}

bool BignumObject::eq(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_bignum() == other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bignum() == other->as_integer()->to_bignum();
    }
    return other->send(env, SymbolObject::intern("=="), { this })->is_truthy();
}

bool BignumObject::lt(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_bignum() < other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bignum() < other->as_integer()->to_bignum();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool BignumObject::lte(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_bignum() <= other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bignum() <= other->as_integer()->to_bignum();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool BignumObject::gt(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_bignum() > other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bignum() > other->as_integer()->to_bignum();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool BignumObject::gte(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_bignum() >= other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        return to_bignum() >= other->as_integer()->to_bignum();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}
}
