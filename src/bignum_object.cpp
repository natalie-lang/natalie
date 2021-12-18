#include "natalie.hpp"

namespace Natalie {

Value BignumObject::to_s(Env *env, Value base_value) {
    if (base_value) {
        // NATFIXME
        NAT_UNREACHABLE();
    }
    return new StringObject { m_bigint->to_string().c_str() };
}

Value BignumObject::add(Env *env, Value arg) {
    if (arg->is_float()) {
        double current = strtod(m_bigint->to_string().c_str(), NULL);
        auto result = current + arg->as_float()->to_double();
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
        double current = strtod(m_bigint->to_string().c_str(), NULL);
        auto result = current - arg->as_float()->to_double();
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
        double current = strtod(m_bigint->to_string().c_str(), NULL);
        auto result = current * arg->as_float()->to_double();
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
        double current = strtod(m_bigint->to_string().c_str(), NULL);
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

    return new BignumObject { to_bigint() / other->to_bigint() };
}

Value BignumObject::negate(Env *env) {
    return new BignumObject { -to_bigint() };
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
