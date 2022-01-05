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
