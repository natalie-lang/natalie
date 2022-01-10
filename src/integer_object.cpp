#include "natalie.hpp"

#include <math.h>

namespace Natalie {

Value IntegerObject::to_s(Env *env, Value base_value) {
    if (m_integer == 0)
        return new StringObject { "0" };
    auto str = new StringObject {};
    nat_int_t base = 10;
    if (base_value) {
        base_value->assert_type(env, Object::Type::Integer, "Integer");
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

Value IntegerObject::to_i() {
    return this;
}

Value IntegerObject::to_f() const {
    return Value { static_cast<double>(m_integer) };
}

Value add_fast(nat_int_t a, nat_int_t b) {
    nat_int_t result = a + b;

    bool overflowed = false;
    if (a < 0 && b < 0 && result > 0)
        overflowed = true;
    if (a > 0 && b > 0 && result < 0)
        overflowed = true;
    if (overflowed) {
        auto result = BigInt(a) + BigInt(b);
        return new BignumObject { result };
    }

    return Value::integer(result);
}

Value IntegerObject::add(Env *env, Value arg) {
    if (arg.is_fast_integer())
        return add_fast(m_integer, arg.get_fast_integer());
    if (arg.is_fast_float())
        return Value { m_integer + arg.get_fast_float() };

    arg.unguard();

    if (arg->is_float()) {
        double result = to_nat_int_t() + arg->as_float()->to_double();
        return new FloatObject { result };
    } else if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    if (other->is_bignum()) {
        auto result = to_bigint() + other->to_bigint();
        return new BignumObject { result };
    }

    return add_fast(m_integer, other->to_nat_int_t());
}

Value sub_fast(nat_int_t a, nat_int_t b) {
    nat_int_t result = a - b;

    bool overflowed = false;
    if (a > 0 && b < 0 && result < 0)
        overflowed = true;
    if (a < 0 && b > 0 && result > 0)
        overflowed = true;
    if (overflowed) {
        auto result = BigInt(a) - BigInt(b);
        return new BignumObject { result };
    }

    return Value::integer(result);
}

Value IntegerObject::sub(Env *env, Value arg) {
    if (arg.is_fast_integer())
        return sub_fast(m_integer, arg.get_fast_integer());
    if (arg.is_fast_float())
        return Value { m_integer - arg.get_fast_float() };

    arg.unguard();

    if (arg->is_float()) {
        double result = to_nat_int_t() - arg->as_float()->to_double();
        return new FloatObject { result };
    } else if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    if (other->is_bignum()) {
        auto result = to_bigint() - other->to_bigint();
        return new BignumObject { result };
    }

    return sub_fast(m_integer, other->to_nat_int_t());
}

bool will_multiplication_overflow(nat_int_t a, nat_int_t b) {
    if (a == b) {
        return a > NAT_MAX_FIXNUM_SQRT || a < -NAT_MAX_FIXNUM_SQRT;
    }

    auto min_fraction = (NAT_MIN_FIXNUM) / b;
    auto max_fraction = (NAT_MAX_FIXNUM) / b;

    return ((a > 0 && b > 0 && max_fraction <= a)
        || (a > 0 && b < 0 && min_fraction <= a)
        || (a < 0 && b > 0 && min_fraction >= a)
        || (a < 0 && b < 0 && max_fraction >= a));
}

Value mul_fast(nat_int_t a, nat_int_t b) {
    if (a == 0 || b == 0)
        return Value::integer(0);

    if (will_multiplication_overflow(a, b)) {
        auto result = BigInt(a) * BigInt(b);
        return new BignumObject { result };
    }

    return Value::integer(a * b);
}
Value IntegerObject::mul(Env *env, Value arg) {
    if (arg.is_fast_integer())
        return mul_fast(m_integer, arg.get_fast_integer());
    if (arg.is_fast_float())
        return Value { m_integer * arg.get_fast_float() };

    arg.unguard();

    if (arg->is_float()) {
        double result = to_nat_int_t() * arg->as_float()->to_double();
        return new FloatObject { result };
    } else if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    if (m_integer == 0 || arg->as_integer()->to_nat_int_t() == 0)
        return Value::integer(0);

    auto other = arg->as_integer();
    if (other->is_bignum()) {
        auto result = to_bigint() * other->to_bigint();
        return new BignumObject { result };
    }

    return mul_fast(m_integer, other->to_nat_int_t());
}

Value div_fast(nat_int_t a, nat_int_t b) {
    nat_int_t res = a / b;
    nat_int_t rem = a % b;
    // Correct division result downwards if up-rounding happened,
    // (for non-zero remainder of sign different than the divisor).
    bool corr = (rem != 0 && ((rem < 0) != (b < 0)));
    return Value::integer(res - corr);
}

Value IntegerObject::div(Env *env, Value arg) {
    if (arg.is_fast_integer() && arg.get_fast_integer() != 0)
        return div_fast(m_integer, arg.get_fast_integer());
    if (arg.is_fast_float())
        return Value { m_integer / arg.get_fast_float() };

    arg.unguard();

    if (arg->is_float()) {
        double result = to_nat_int_t() / arg->as_float()->to_double();
        return new FloatObject { result };
    } else if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer();
    if (other->is_bignum()) {
        auto other = arg->as_integer();
        auto result = to_bigint() / other->to_bigint();
        return new BignumObject { result };
    }

    if (other->to_nat_int_t() == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    return div_fast(m_integer, other->to_nat_int_t());
}

Value IntegerObject::mod(Env *env, Value arg) {
    // FIXME: add mod_fast, like div_fast
    arg.unguard();

    if (arg->is_float())
        arg = Value::integer(arg->as_float()->to_double());

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto nat_int = arg->as_integer()->to_nat_int_t();
    if (nat_int == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    // Cast operands to floats to get decimal result from division.
    auto f = ::floor((float)m_integer / (float)nat_int);
    auto result = m_integer - (nat_int * f);

    return Value::integer(result);
}

Value fast_pow(Env *env, nat_int_t a, nat_int_t b) {
    if (b == 0)
        return Value::integer(1);
    if (b == 1)
        return Value::integer(a);
    if (a == 0)
        return Value::integer(0);

    auto handle_overflow = [env, &a, &b](nat_int_t last_result, bool should_be_negative) -> Value {
        auto result = BignumObject(a).pow(env, Value::integer(b));

        // Check if the bignum pow overflowed.
        if (result->is_float() && result->as_float()->is_infinite(env)) {
            return result;
        }

        result = result->as_integer()->mul(env, Value::integer(should_be_negative ? -last_result : last_result));
        return result;
    };

    nat_int_t result = 1;

    bool negative = a < 0;
    if (negative) a = -a;
    negative = negative && (b % 2);

    while (b != 0) {
        while (b % 2 == 0) {
            if (a > NAT_MAX_FIXNUM_SQRT) {
                return handle_overflow(result, negative);
            }
            b >>= 1;
            a *= a;
        }

        if (will_multiplication_overflow(result, a)) {
            return handle_overflow(result, negative);
        }
        result *= a;
        --b;
    }
    return Value::integer(negative ? -result : result);
}

Value IntegerObject::pow(Env *env, Value arg) {
    nat_int_t nat_int;
    if (arg.is_fast_integer()) {
        nat_int = arg.get_fast_integer();
    } else {
        arg.unguard();

        if (arg->is_float())
            return FloatObject { to_nat_int_t() }.pow(env, arg);

        if (!arg->is_integer()) {
            auto coerced = Natalie::coerce(env, arg, this);
            arg = coerced.second;
            if (!coerced.first->is_integer()) {
                return coerced.first->send(env, "**"_s, { arg });
            }
        }

        arg->assert_type(env, Object::Type::Integer, "Integer");

        if (arg->as_integer()->is_bignum())
            return BignumObject { to_nat_int_t() }.pow(env, arg);

        nat_int = arg->as_integer()->to_nat_int_t();
    }

    if (m_integer == 0 && nat_int < 0)
        env->raise("ZeroDivisionError", "divided by 0");

    // NATFIXME: If a negative number is passed we want to return a Rational
    if (nat_int < 0)
        NAT_NOT_YET_IMPLEMENTED();

    return fast_pow(env, m_integer, nat_int);
}

Value IntegerObject::cmp(Env *env, Value arg) {
    auto is_comparable_with = [](Value arg) -> bool {
        return arg.is_fast_integer() || arg.is_fast_float() || arg->is_integer() || arg->is_float();
    };

    // Check if we might want to coerce the value
    if (!is_comparable_with(arg))
        arg = Natalie::coerce(env, arg, this, Natalie::CoerceInvalidReturnValueMode::Allow).second;

    // Check if compareable
    if (!is_comparable_with(arg))
        return NilObject::the();

    if (lt(env, arg)) {
        return Value::integer(-1);
    } else if (eq(env, arg)) {
        return Value::integer(0);
    } else {
        return Value::integer(1);
    }
}

bool IntegerObject::eq(Env *env, Value other) {
    if (other.is_fast_integer())
        return m_integer == other.get_fast_integer();
    if (other.is_fast_float())
        return m_integer == other.get_fast_float();

    other.unguard();

    if (other->is_float()) {
        return to_nat_int_t() == other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bigint() == other->as_integer()->to_bigint();
        return to_nat_int_t() == other->as_integer()->to_nat_int_t();
    }
    return other->send(env, "=="_s, { this })->is_truthy();
}

bool IntegerObject::lt(Env *env, Value other) {
    if (other.is_fast_integer())
        return m_integer < other.get_fast_integer();
    if (other.is_fast_float())
        return m_integer < other.get_fast_float();

    other.unguard();

    if (other->is_float()) {
        return to_nat_int_t() < other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bigint() < other->as_integer()->to_bigint();
        return to_nat_int_t() < other->as_integer()->to_nat_int_t();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::lte(Env *env, Value other) {
    if (other.is_fast_integer())
        return m_integer <= other.get_fast_integer();
    if (other.is_fast_float())
        return m_integer <= other.get_fast_float();

    other.unguard();

    if (other->is_float()) {
        return to_nat_int_t() <= other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bigint() <= other->as_integer()->to_bigint();
        return to_nat_int_t() <= other->as_integer()->to_nat_int_t();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::gt(Env *env, Value other) {
    if (other.is_fast_integer())
        return m_integer > other.get_fast_integer();
    if (other.is_fast_float())
        return m_integer > other.get_fast_float();

    other.unguard();

    if (other->is_float()) {
        return to_nat_int_t() > other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bigint() > other->as_integer()->to_bigint();
        return to_nat_int_t() > other->as_integer()->to_nat_int_t();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::gte(Env *env, Value other) {
    if (other.is_fast_integer())
        return m_integer >= other.get_fast_integer();
    if (other.is_fast_float())
        return m_integer >= other.get_fast_float();

    other.unguard();

    if (other->is_float()) {
        return to_nat_int_t() >= other->as_float()->to_double();
    }

    if (!other->is_integer()) {
        other = Natalie::coerce(env, other, this).second;
    }

    if (other->is_integer()) {
        if (other->as_integer()->is_bignum())
            return to_bigint() >= other->as_integer()->to_bigint();
        return to_nat_int_t() >= other->as_integer()->to_nat_int_t();
    }
    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

Value IntegerObject::times(Env *env, Block *block) {
    auto val = to_nat_int_t();
    if (!block) {
        auto enumerator = send(env, "enum_for"_s, { "times"_s });
        enumerator->ivar_set(env, "@size"_s, val < 0 ? Value::integer(0) : this);
        return enumerator;
    }

    if (val <= 0)
        return this;

    for (nat_int_t i = 0; i < val; i++) {
        Value num = Value::integer(i);
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &num, nullptr);
    }
    return this;
}

Value IntegerObject::bitwise_and(Env *env, Value arg) const {
    arg->assert_type(env, Object::Type::Integer, "Integer");
    return Value::integer(to_nat_int_t() & arg->as_integer()->to_nat_int_t());
}

Value IntegerObject::bitwise_or(Env *env, Value arg) const {
    arg->assert_type(env, Object::Type::Integer, "Integer");
    return Value::integer(to_nat_int_t() | arg->as_integer()->to_nat_int_t());
}

Value IntegerObject::pred(Env *env) {
    return sub(env, Value::integer(1));
}

Value IntegerObject::size(Env *env) {
    // NATFIXME: add Bignum support.
    return Value::integer(sizeof m_integer);
}

Value IntegerObject::succ(Env *env) {
    return add(env, Value::integer(1));
}

Value IntegerObject::coerce(Env *env, Value arg) {
    ArrayObject *ary = new ArrayObject {};
    switch (arg->type()) {
    case Object::Type::Integer:
        ary->push(arg);
        ary->push(this);
        break;
    case Object::Type::String:
        ary->push(send(env, "Float"_s, { arg }));
        ary->push(send(env, "to_f"_s));
        break;
    default:
        if (!arg->is_nil() && !arg->is_float() && arg->respond_to(env, "to_f"_s)) {
            arg = arg.send(env, "to_f"_s);
        }

        if (arg->is_float()) {
            ary->push(arg);
            ary->push(send(env, "to_f"_s));
            break;
        }

        env->raise("TypeError", "can't convert {} into Float", arg->inspect_str(env));
    }
    return ary;
}

Value IntegerObject::ceil(Env *env, Value arg) {
    if (arg == nullptr)
        return this;

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto precision = arg->as_integer()->to_nat_int_t();
    if (precision >= 0)
        return this;

    double f = ::pow(10, precision);
    auto result = ::ceil(to_nat_int_t() * f) / f;

    return Value::integer(result);
}

Value IntegerObject::floor(Env *env, Value arg) {
    if (arg == nullptr)
        return this;

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto precision = arg->as_integer()->to_nat_int_t();
    if (precision >= 0)
        return this;

    double f = ::pow(10, precision);
    auto result = ::floor(to_nat_int_t() * f) / f;

    return Value::integer(result);
}

Value gcd_fast(nat_int_t a, nat_int_t b) {
    auto this_abs = ::abs(a);
    auto divisor_abs = ::abs(b);
    auto remainder = divisor_abs;

    while (remainder != 0) {
        remainder = this_abs % divisor_abs;
        this_abs = divisor_abs;
        divisor_abs = remainder;
    }

    return Value::integer(this_abs);
}

Value IntegerObject::gcd(Env *env, Value divisor) {
    if (divisor.is_fast_integer())
        return gcd_fast(m_integer, divisor.get_fast_integer());

    divisor->assert_type(env, Object::Type::Integer, "Integer");

    auto other = divisor->as_integer();
    if (other->is_bignum()) {
        auto result = ::gcd(to_bigint(), other->to_bigint());
        return new BignumObject { result };
    }

    return gcd_fast(m_integer, other->to_nat_int_t());
}

bool IntegerObject::eql(Env *env, Value other) {
    if (other.is_fast_integer())
        return m_integer == other.get_fast_integer();
    if (other.is_fast_float())
        return false;

    other.unguard();

    return other->is_integer() && other->as_integer()->to_nat_int_t() == to_nat_int_t();
}

Value IntegerObject::abs(Env *env) {
    auto number = to_nat_int_t();
    if (number < 0) {
        return Value::integer(-1 * number);
    } else {
        return this;
    }
}

Value IntegerObject::chr(Env *env) const {
    char c = static_cast<char>(to_nat_int_t());
    char str[] = " ";
    str[0] = c;
    return new StringObject { str };
}

bool IntegerObject::optimized_method(SymbolObject *method_name) {
    if (s_optimized_methods.is_empty()) {
        // NOTE: No method that ever returns 'this' can be an "optimized" method. Trust me!
        s_optimized_methods.set("+"_s);
        s_optimized_methods.set("-"_s);
        s_optimized_methods.set("*"_s);
        s_optimized_methods.set("/"_s);
        s_optimized_methods.set("%"_s);
        s_optimized_methods.set("**"_s);
        s_optimized_methods.set("<=>"_s);
        s_optimized_methods.set("=="_s);
        s_optimized_methods.set("==="_s);
        s_optimized_methods.set("<"_s);
        s_optimized_methods.set("<="_s);
        s_optimized_methods.set(">"_s);
        s_optimized_methods.set(">="_s);
        s_optimized_methods.set("==="_s);
        s_optimized_methods.set("eql?"_s);
        s_optimized_methods.set("succ"_s);
        s_optimized_methods.set("chr"_s);
        s_optimized_methods.set("~"_s);
    }
    return !!s_optimized_methods.get(method_name);
}

Value IntegerObject::negate(Env *env) {
    return Value::integer(-1 * m_integer);
}

Value IntegerObject::numerator() {
    return this;
}

Value IntegerObject::complement(Env *env) const {
    return Value::integer(~m_integer);
}
}
