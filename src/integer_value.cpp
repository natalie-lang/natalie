#include "natalie.hpp"

#include <math.h>

namespace Natalie {

ValuePtr IntegerValue::to_s(Env *env, ValuePtr base_value) {
    if (m_integer == 0)
        return new StringValue { "0" };
    auto str = new StringValue {};
    nat_int_t base = 10;
    if (base_value) {
        base_value->assert_type(env, Value::Type::Integer, "Integer");
        base = base_value->as_integer()->to_nat_int_t();

        if (base < 2 || base > 36) {
            env->raise("ArgumentError", "invalid radix {}", base);
        }
    }
    auto num = m_integer;
    bool negative = false;
    if (num < 0) {
        negative = true;
        num *= -1;
    }
    while (num > 0) {
        auto digit = num % base;
        char c;
        if (digit >= 0 && digit <= 9)
            c = digit + 48;
        else
            c = digit + 87;
        str->prepend_char(env, c);
        num /= base;
    }
    if (negative)
        str->prepend_char(env, '-');
    return str;
}

ValuePtr IntegerValue::to_i() {
    return this;
}

ValuePtr IntegerValue::to_f() {
    return new FloatValue { m_integer };
}

ValuePtr IntegerValue::add(Env *env, ValuePtr arg) {
    if (arg.is_float()) {
        double result = to_nat_int_t() + arg->as_float()->to_double();
        return new FloatValue { result };
    } else if (!arg.is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }
    arg.assert_type(env, Value::Type::Integer, "Integer");

    auto other = arg->as_integer();
    if (other->is_bignum()) {
        auto result = to_bignum() + other->to_bignum();
        return new BignumValue { result };
    }

    nat_int_t result = to_nat_int_t() + arg.to_nat_int_t();
    bool overflowed = false;
    if (to_nat_int_t() < 0 && arg.to_nat_int_t() < 0 && result > 0)
        overflowed = true;
    if (to_nat_int_t() > 0 && arg.to_nat_int_t() > 0 && result < 0)
        overflowed = true;

    if (overflowed) {
        auto result = to_bignum() + other->to_bignum();
        return new BignumValue { result };
    }
    return ValuePtr::integer(result);
}

ValuePtr IntegerValue::sub(Env *env, ValuePtr arg) {
    if (arg.is_float()) {
        double result = to_nat_int_t() - arg->as_float()->to_double();
        return new FloatValue { result };
    }
    arg.assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t result = to_nat_int_t() - arg.to_nat_int_t();
    return ValuePtr::integer(result);
}

ValuePtr IntegerValue::mul(Env *env, ValuePtr arg) {
    if (arg.is_float()) {
        double result = to_nat_int_t() * arg->as_float()->to_double();
        return new FloatValue { result };
    }
    arg.assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t result = to_nat_int_t() * arg.to_nat_int_t();
    return ValuePtr::integer(result);
}

ValuePtr IntegerValue::div(Env *env, ValuePtr arg) {
    if (arg.is_integer()) {
        nat_int_t dividend = to_nat_int_t();
        nat_int_t divisor = arg.to_nat_int_t();
        if (divisor == 0) {
            env->raise("ZeroDivisionError", "divided by 0");
        }
        nat_int_t result = dividend / divisor;
        return ValuePtr::integer(result);

    } else if (arg->respond_to(env, SymbolValue::intern("coerce"))) {
        auto coerce_result = Natalie::coerce(env, arg, this);

        ValuePtr dividend = coerce_result.first;
        ValuePtr divisor = coerce_result.second;
        return dividend.send(env, SymbolValue::intern("/"), { divisor });
    } else {
        arg->assert_type(env, Value::Type::Integer, "Integer");
        return nullptr;
    }
}

ValuePtr IntegerValue::mod(Env *env, ValuePtr arg) {
    arg.assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t result = to_nat_int_t() % arg.to_nat_int_t();
    return ValuePtr::integer(result);
}

ValuePtr IntegerValue::pow(Env *env, ValuePtr arg) {
    arg.assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t result = ::pow(to_nat_int_t(), arg.to_nat_int_t());
    return ValuePtr::integer(result);
}

ValuePtr IntegerValue::cmp(Env *env, ValuePtr arg) {
    if (!arg.is_integer() && !arg.is_float()) return NilValue::the();
    if (lt(env, arg)) {
        return ValuePtr::integer(-1);
    } else if (eq(env, arg)) {
        return ValuePtr::integer(0);
    } else {
        return ValuePtr::integer(1);
    }
}

bool IntegerValue::eq(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_nat_int_t() == other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other.is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bignum() == other->as_integer()->to_bignum();
        return to_nat_int_t() == other.to_nat_int_t();
    }
    return other->send(env, SymbolValue::intern("=="), { this })->is_truthy();
}

bool IntegerValue::lt(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_nat_int_t() < other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other.is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bignum() < other->as_integer()->to_bignum();
        return to_nat_int_t() < other.to_nat_int_t();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerValue::lte(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_nat_int_t() <= other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other.is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bignum() <= other->as_integer()->to_bignum();
        return to_nat_int_t() <= other.to_nat_int_t();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerValue::gt(Env *env, ValuePtr other) {
    if (other->is_float()) {
        return to_nat_int_t() > other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other.is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bignum() > other->as_integer()->to_bignum();
        return to_nat_int_t() > other.to_nat_int_t();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerValue::gte(Env *env, ValuePtr other) {
    if (other.is_float()) {
        return to_nat_int_t() >= other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other.is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bignum() >= other->as_integer()->to_bignum();
        return to_nat_int_t() >= other.to_nat_int_t();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

ValuePtr IntegerValue::eqeqeq(Env *env, ValuePtr arg) {
    if (arg.is_integer() && to_nat_int_t() == arg.to_nat_int_t()) {
        return TrueValue::the();
    } else {
        return FalseValue::the();
    }
}

ValuePtr IntegerValue::times(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("times") });

    nat_int_t val = to_nat_int_t();
    assert(val >= 0);
    env->ensure_block_given(block);
    ValuePtr num;
    for (nat_int_t i = 0; i < val; i++) {
        ValuePtr num = ValuePtr::integer(i);
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &num, nullptr);
    }
    return this;
}

ValuePtr IntegerValue::bitwise_and(Env *env, ValuePtr arg) {
    arg.assert_type(env, Value::Type::Integer, "Integer");
    return ValuePtr::integer(to_nat_int_t() & arg.to_nat_int_t());
}

ValuePtr IntegerValue::bitwise_or(Env *env, ValuePtr arg) {
    arg.assert_type(env, Value::Type::Integer, "Integer");
    return ValuePtr::integer(to_nat_int_t() | arg.to_nat_int_t());
}

ValuePtr IntegerValue::succ(Env *env) {
    return ValuePtr::integer(to_nat_int_t() + 1);
}

ValuePtr IntegerValue::coerce(Env *env, ValuePtr arg) {
    ArrayValue *ary = new ArrayValue {};
    switch (arg->type()) {
    case Value::Type::Float:
        ary->push(arg);
        ary->push(new FloatValue { to_nat_int_t() });
        break;
    case Value::Type::Integer:
        ary->push(arg);
        ary->push(this);
        break;
    case Value::Type::String:
        printf("TODO\n");
        abort();
        break;
    default:
        env->raise("ArgumentError", "invalid value for Float(): {}", arg->inspect_str(env));
    }
    return ary;
}

bool IntegerValue::eql(Env *env, ValuePtr other) {
    return other.is_integer() && other.to_nat_int_t() == to_nat_int_t();
}

ValuePtr IntegerValue::abs(Env *env) {
    auto number = to_nat_int_t();
    if (number < 0) {
        return ValuePtr::integer(-1 * number);
    } else {
        return this;
    }
}

ValuePtr IntegerValue::chr(Env *env) {
    char c = static_cast<char>(to_nat_int_t());
    char str[] = " ";
    str[0] = c;
    return new StringValue { str };
}

bool IntegerValue::optimized_method(SymbolValue *method_name) {
    if (s_optimized_methods.is_empty()) {
        // NOTE: No method that ever returns 'this' can be an "optimized" method. Trust me!
        s_optimized_methods.set(SymbolValue::intern("+"));
        s_optimized_methods.set(SymbolValue::intern("-"));
        s_optimized_methods.set(SymbolValue::intern("*"));
        s_optimized_methods.set(SymbolValue::intern("/"));
        s_optimized_methods.set(SymbolValue::intern("%"));
        s_optimized_methods.set(SymbolValue::intern("**"));
        s_optimized_methods.set(SymbolValue::intern("<=>"));
        s_optimized_methods.set(SymbolValue::intern("=="));
        s_optimized_methods.set(SymbolValue::intern("==="));
        s_optimized_methods.set(SymbolValue::intern("<"));
        s_optimized_methods.set(SymbolValue::intern("<="));
        s_optimized_methods.set(SymbolValue::intern(">"));
        s_optimized_methods.set(SymbolValue::intern(">="));
        s_optimized_methods.set(SymbolValue::intern("==="));
        s_optimized_methods.set(SymbolValue::intern("eql?"));
        s_optimized_methods.set(SymbolValue::intern("succ"));
        s_optimized_methods.set(SymbolValue::intern("chr"));
        s_optimized_methods.set(SymbolValue::intern("~"));
    }
    return !!s_optimized_methods.get(method_name);
}

ValuePtr IntegerValue::negate(Env *env) {
    return ValuePtr::integer(-1 * m_integer);
}

ValuePtr IntegerValue::complement(Env *env) {
    return ValuePtr::integer(~m_integer);
}
}
