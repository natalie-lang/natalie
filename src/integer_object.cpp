#include "natalie.hpp"

#include <math.h>

namespace Natalie {

Value IntegerObject::create(const Integer &integer) {
    if (integer.is_bignum())
        return new IntegerObject { integer };
    return Value::integer(integer.to_nat_int_t());
}

Value IntegerObject::create(const char *string) {
    return new IntegerObject { BigInt(string) };
};

Value IntegerObject::to_s(Env *env, Value base_value) const {
    if (m_integer == 0)
        return new StringObject { "0" };

    nat_int_t base = 10;
    if (base_value) {
        base = convert_to_nat_int_t(env, base_value);

        if (base < 2 || base > 36) {
            env->raise("ArgumentError", "invalid radix {}", base);
        }
    }

    if (base == 10)
        return new StringObject { m_integer.to_string(), Encoding::US_ASCII };

    auto str = new StringObject { "", Encoding::US_ASCII };
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
            c = digit.to_nat_int_t() + '0';
        else
            c = digit.to_nat_int_t() + 'a' - 10;
        str->prepend_char(env, c);
        num /= base;
    }
    if (negative)
        str->prepend_char(env, '-');
    return str;
}

Value IntegerObject::to_i() const {
    return IntegerObject::create(m_integer);
}

Value IntegerObject::to_f() const {
    return Value::floatingpoint(m_integer.to_double());
}

Value IntegerObject::add(Env *env, Value arg) {
    if (arg->is_float()) {
        return Value::floatingpoint(m_integer + arg->as_float()->to_double());
    } else if (arg->is_complex()) {
        return arg->send(env, "+"_s, { this });
    } else if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    return create(m_integer + arg->as_integer()->integer());
}

Value IntegerObject::sub(Env *env, Value arg) {
    if (arg->is_float()) {
        double result = m_integer.to_double() - arg->as_float()->to_double();
        return Value::floatingpoint(result);
    } else if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    return create(m_integer - arg->as_integer()->integer());
}

Value IntegerObject::mul(Env *env, Value arg) {
    if (arg->is_float()) {
        double result = m_integer.to_double() * arg->as_float()->to_double();
        return new FloatObject { result };
    } else if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    if (m_integer == 0 || arg->as_integer()->integer() == 0)
        return Value::integer(0);

    return create(m_integer * arg->as_integer()->integer());
}

Value IntegerObject::div(Env *env, Value arg) {
    if (arg->is_float()) {
        double result = m_integer / arg->as_float()->to_double();
        if (isnan(result))
            return FloatObject::nan();
        return Value::floatingpoint(result);
    } else if (arg->is_rational()) {
        return RationalObject(this, new IntegerObject { 1 }).div(env, arg);
    } else if (!arg->is_integer()) {
        arg = Natalie::coerce(env, arg, this).second;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg->as_integer()->integer();
    if (other == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    return create(m_integer / other);
}

Value IntegerObject::mod(Env *env, Value arg) {
    Integer argument;
    if (arg->is_float()) {
        return Value::floatingpoint(m_integer.to_double())->as_float()->mod(env, arg);
    } else if (arg->is_rational()) {
        return RationalObject { this, new IntegerObject { 1 } }.send(env, "%"_s, { arg });
    } else {
        arg->assert_type(env, Object::Type::Integer, "Integer");
        argument = arg->as_integer()->integer();
    }

    if (argument == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    return create(m_integer % argument);
}

Value IntegerObject::pow(Env *env, Value arg) {
    if (arg->is_float())
        return Value::floatingpoint(m_integer.to_double())->as_float()->pow(env, arg);

    if (arg->is_rational())
        return RationalObject { this, new IntegerObject { 1 } }.pow(env, arg);

    if (!arg->is_integer()) {
        auto coerced = Natalie::coerce(env, arg, this);
        arg = coerced.second;
        if (!coerced.first->is_integer()) {
            return coerced.first->send(env, "**"_s, { arg });
        }
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto arg_int = arg->as_integer()->integer();

    if (m_integer == 0 && arg_int < 0)
        env->raise("ZeroDivisionError", "divided by 0");

    // NATFIXME: If a negative number is passed we want to return a Rational
    if (arg_int < 0) {
        auto denominator = Natalie::pow(m_integer, -arg_int);
        return new RationalObject { new IntegerObject { 1 }, new IntegerObject { denominator } };
    }

    if (arg_int == 0)
        return Value::integer(1);
    else if (arg_int == 1)
        return create(m_integer);

    if (m_integer == 0)
        return Value::integer(0);
    else if (m_integer == 1)
        return Value::integer(1);
    else if (m_integer == -1)
        return Value::integer(arg_int % 2 ? 1 : -1);

    // NATFIXME: Ruby has a really weird max bignum value that is calculated by the words needed to store a bignum
    // The calculation that we do is pretty much guessed to be in the direction of ruby. However, we should do more research about this limit...
    size_t length = m_integer.to_string().length();
    constexpr const size_t BIGINT_LIMIT = 8 * 1024 * 1024;
    if (length > BIGINT_LIMIT || length * arg_int > (nat_int_t)BIGINT_LIMIT) {
        env->warn("in a**b, b may be too big");
        return Value::floatingpoint(m_integer.to_double())->as_float()->pow(env, arg);
    }

    return create(Natalie::pow(m_integer, arg_int));
}

Value IntegerObject::powmod(Env *env, Value exponent, Value mod) {
    if (exponent->is_integer() && exponent->as_integer()->to_nat_int_t() < 0 && mod)
        env->raise("RangeError", "2nd argument not allowed when first argument is negative");

    auto powd = pow(env, exponent);

    if (!mod)
        return powd;

    if (!mod->is_integer())
        env->raise("TypeError", "2nd argument not allowed unless all arguments are integers");

    auto modi = mod->as_integer();
    if (modi->to_nat_int_t() == 0)
        env->raise("ZeroDivisionError", "cannot divide by zero");

    auto powi = powd->as_integer();

    if (powi->is_bignum())
        return new IntegerObject { powi->to_bigint() % modi->to_bigint() };

    if (powi->to_nat_int_t() < 0 || modi->to_nat_int_t() < 0)
        return powi->mod(env, mod);

    return Value::integer(powi->to_nat_int_t() % modi->to_nat_int_t());
}

Value IntegerObject::cmp(Env *env, Value arg) {
    auto is_comparable_with = [](Value arg) -> bool {
        return arg->is_integer() || arg->is_float() || arg->is_rational();
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
    if (other->is_float())
        return m_integer == other->as_float()->to_double();

    if (other->is_integer())
        return m_integer == other->as_integer()->integer();

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, "=="_s, { result.second })->is_truthy();
    }

    return other->send(env, "=="_s, { this })->is_truthy();
}

bool IntegerObject::lt(Env *env, Value other) {
    if (other->is_float())
        return m_integer < other->as_float()->to_double();

    if (other->is_integer())
        return m_integer < other->as_integer()->integer();

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, "<"_s, { result.second })->is_truthy();
    }

    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::lte(Env *env, Value other) {
    if (other->is_float())
        return m_integer <= other->as_float()->to_double();

    if (other->is_integer())
        return m_integer <= other->as_integer()->integer();

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, "<="_s, { result.second })->is_truthy();
    }

    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::gt(Env *env, Value other) {
    if (other->is_float())
        return m_integer > other->as_float()->to_double();

    if (other->is_integer())
        return m_integer > other->as_integer()->integer();

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, ">"_s, { result.second })->is_truthy();
    }

    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::gte(Env *env, Value other) {
    if (other->is_float())
        return m_integer >= other->as_float()->to_double();

    if (other->is_integer())
        return m_integer >= other->as_integer()->integer();

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, this, Natalie::CoerceInvalidReturnValueMode::Raise);
        return result.first->send(env, ">="_s, { result.second })->is_truthy();
    }

    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

Value IntegerObject::times(Env *env, Block *block) {
    if (!block) {
        auto enumerator = send(env, "enum_for"_s, { "times"_s });
        enumerator->ivar_set(env, "@size"_s, m_integer < 0 ? Value::integer(0) : this);
        return enumerator;
    }

    if (m_integer <= 0)
        return IntegerObject::create(m_integer);

    for (Integer i = 0; i < m_integer; ++i) {
        Value num = create(i);
        Value args[] = { num };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    }
    return IntegerObject::create(m_integer);
}

Value IntegerObject::bitwise_and(Env *env, Value arg) {
    if (!arg->is_integer()) {
        auto result = Natalie::coerce(env, arg, this);
        result.second->assert_type(env, Object::Type::Integer, "Integer");
        return result.first.send(env, "&"_s, { result.second });
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    return create(m_integer & arg->as_integer()->integer());
}

Value IntegerObject::bitwise_or(Env *env, Value arg) {
    Integer argument;
    if (!arg->is_integer()) {
        auto result = Natalie::coerce(env, arg, this);
        result.second->assert_type(env, Object::Type::Integer, "Integer");
        return result.first.send(env, "|"_s, { result.second });
    } else {
        arg->assert_type(env, Object::Type::Integer, "Integer");
        argument = arg->as_integer()->integer();
    }

    return create(m_integer | argument);
}

Value IntegerObject::bitwise_xor(Env *env, Value arg) {
    Integer argument;
    if (!arg->is_integer()) {
        auto result = Natalie::coerce(env, arg, this);
        result.second->assert_type(env, Object::Type::Integer, "Integer");
        return result.first.send(env, "^"_s, { result.second });
    } else {
        arg->assert_type(env, Object::Type::Integer, "Integer");
        argument = arg->as_integer()->integer();
    }

    return create(m_integer ^ argument);
}

Value IntegerObject::bit_length(Env *env) {
    return create(m_integer.bit_length());
}

Value IntegerObject::left_shift(Env *env, Value arg) {
    auto integer = arg->to_int(env);
    if (integer->is_bignum()) {
        if (is_negative())
            return Value::integer(-1);
        else
            return Value::integer(0);
    }

    auto nat_int = integer->integer().to_nat_int_t();

    if (nat_int < 0)
        return right_shift(env, Value::integer(-nat_int));

    return create(m_integer << nat_int);
}

Value IntegerObject::right_shift(Env *env, Value arg) {
    auto integer = arg->to_int(env);
    if (integer->is_bignum()) {
        if (is_negative())
            return Value::integer(-1);
        else
            return Value::integer(0);
    }

    auto nat_int = integer->integer().to_nat_int_t();

    if (nat_int < 0)
        return left_shift(env, Value::integer(-nat_int));

    return create(m_integer >> nat_int);
}

Value IntegerObject::pred(Env *env) {
    return sub(env, Value::integer(1));
}

Value IntegerObject::size(Env *env) {
    // NATFIXME: add Bignum support.
    return Value::integer(sizeof(nat_int_t));
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
        return IntegerObject::create(m_integer);

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto precision = arg->as_integer()->integer().to_nat_int_t();
    if (precision >= 0)
        return IntegerObject::create(m_integer);

    double f = ::pow(10, precision);
    auto result = ::ceil(m_integer.to_nat_int_t() * f) / f;

    return Value::integer(result);
}

Value IntegerObject::floor(Env *env, Value arg) {
    if (arg == nullptr)
        return IntegerObject::create(m_integer);

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto precision = arg->as_integer()->integer().to_nat_int_t();
    if (precision >= 0)
        return IntegerObject::create(m_integer);

    double f = ::pow(10, precision);
    auto result = ::floor(m_integer.to_nat_int_t() * f) / f;

    return Value::integer(result);
}

Value IntegerObject::gcd(Env *env, Value divisor) {
    divisor->assert_type(env, Object::Type::Integer, "Integer");
    return create(Natalie::gcd(m_integer, divisor->as_integer()->integer()));
}

Value IntegerObject::abs(Env *env) {
    if (m_integer.is_negative())
        return create(-m_integer);
    return IntegerObject::create(m_integer);
}

Value IntegerObject::chr(Env *env, Value encoding) const {
    if (m_integer < 0 || m_integer > (nat_int_t)UINT_MAX)
        env->raise("RangeError", "{} out of char range", m_integer.to_string());
    else if (is_bignum())
        env->raise("RangeError", "bignum out of char range");

    if (encoding) {
        if (!encoding->is_encoding()) {
            encoding->assert_type(env, Type::String, "String");
            encoding = EncodingObject::find(env, encoding);
        }
    } else if (m_integer <= 127) {
        encoding = EncodingObject::get(Encoding::US_ASCII);
    } else if (m_integer < 256) {
        encoding = EncodingObject::get(Encoding::ASCII_8BIT);
    } else if (EncodingObject::default_internal()) {
        encoding = EncodingObject::default_internal();
    } else {
        env->raise("RangeError", "{} out of char range", m_integer.to_string());
    }

    auto encoding_obj = encoding->as_encoding();
    if (!encoding_obj->in_encoding_codepoint_range(m_integer.to_nat_int_t()))
        env->raise("RangeError", "{} out of char range", m_integer.to_string());

    if (!encoding_obj->valid_codepoint(m_integer.to_nat_int_t())) {
        auto hex = String();
        hex.append_sprintf("0x%X", m_integer.to_nat_int_t());

        auto encoding_name = encoding_obj->name()->as_string()->string();
        env->raise("RangeError", "invalid codepoint {} in {}", hex, encoding_name);
    }

    auto encoded = encoding_obj->encode_codepoint(m_integer.to_nat_int_t());
    return new StringObject { encoded, encoding_obj };
}

Value IntegerObject::negate(Env *env) {
    return create(-m_integer);
}

Value IntegerObject::numerator() {
    return IntegerObject::create(m_integer);
}

Value IntegerObject::complement(Env *env) const {
    return create(~m_integer);
}

Value IntegerObject::sqrt(Env *env, Value arg) {
    auto argument = arg->to_int(env)->integer();

    if (argument < 0) {
        auto domain_error = fetch_nested_const({ "Math"_s, "DomainError"_s });
        auto message = new StringObject { "Numerical argument is out of domain - \"isqrt\"" };
        auto exception = new ExceptionObject { domain_error->as_class(), message };
        env->raise_exception(exception);
    }

    return create(Natalie::sqrt(argument));
}

Value IntegerObject::quo(Env *env, Value quotient) {
    auto rat = new RationalObject { this->as_integer(), new IntegerObject { 1 } };
    return rat->div(env, quotient);
}

Value IntegerObject::round(Env *env, Value ndigits, Value half) {
    if (!ndigits)
        return IntegerObject::create(m_integer);

    int digits = IntegerObject::convert_to_int(env, ndigits);
    RoundingMode rounding_mode = rounding_mode_from_value(env, half);

    if (digits >= 0)
        return IntegerObject::create(m_integer);

    auto result = m_integer;
    auto dividend = Natalie::pow(Integer(10), -digits);

    auto dividend_half = dividend / 2;
    auto remainder = result.modulo_c(dividend);
    auto remainder_abs = Natalie::abs(remainder);

    if (remainder_abs < dividend_half) {
        result -= remainder;
    } else if (remainder_abs > dividend_half) {
        result += dividend - remainder;
    } else {
        switch (rounding_mode) {
        case RoundingMode::Up:
            result += remainder;
            break;
        case RoundingMode::Down:
            result -= remainder;
            break;
        case RoundingMode::Even:
            auto digit = result.modulo_c(dividend * 10).div_c(dividend);
            if (digit % 2 == 0) {
                result -= remainder;
            } else {
                result += remainder;
            }
            break;
        }
    }

    return create(result);
}

Value IntegerObject::truncate(Env *env, Value ndigits) {
    if (!ndigits)
        return IntegerObject::create(m_integer);

    int digits = IntegerObject::convert_to_int(env, ndigits);

    if (digits >= 0)
        return IntegerObject::create(m_integer);

    auto result = m_integer;
    auto dividend = Natalie::pow(Integer(10), -digits);
    auto remainder = result.modulo_c(dividend);

    return create(result - remainder);
}

Value IntegerObject::ref(Env *env, Value offset_obj, Value size_obj) {
    auto from_offset_and_size = [this, env](Optional<nat_int_t> offset_or_empty, Optional<nat_int_t> size_or_empty = {}) -> Value {
        auto offset = offset_or_empty.value_or(0);

        if (!size_or_empty.present() && offset < 0)
            return Value::integer(0);

        auto size = size_or_empty.value_or(1);

        Integer result;
        if (offset < 0)
            result = m_integer << -offset;
        else
            result = m_integer >> offset;

        if (size >= 0)
            result = result & ((1 << size) - 1);

        if (result != 0 && !offset_or_empty.present())
            env->raise("ArgumentError", "The beginless range for Integer#[] results in infinity");

        return create(result);
    };

    if (!size_obj && offset_obj->is_range()) {
        auto range = offset_obj->as_range();

        Optional<nat_int_t> begin;
        if (!range->begin()->is_nil()) {
            auto begin_obj = range->begin()->to_int(env);
            begin = begin_obj->integer().to_nat_int_t();
        }

        Optional<nat_int_t> end;
        if (!range->end()->is_nil()) {
            auto end_obj = range->end()->to_int(env);
            end = end_obj->integer().to_nat_int_t();
        }

        Optional<nat_int_t> size;
        if (!end || (begin && end.value() < begin.value()))
            size = -1;
        else if (end)
            size = end.value() - begin.value_or(0) + 1;

        return from_offset_and_size(begin, size);
    } else {
        auto *offset_integer = offset_obj->to_int(env);
        if (offset_integer->is_bignum())
            return Value::integer(0);

        auto offset = offset_integer->integer().to_nat_int_t();

        Optional<nat_int_t> size;
        if (size_obj) {
            IntegerObject *size_integer = size_obj->to_int(env);
            if (size_integer->is_bignum())
                env->raise("RangeError", "shift width too big");

            size = size_integer->integer().to_nat_int_t();
        }

        return from_offset_and_size(offset, size);
    }
}

nat_int_t IntegerObject::convert_to_nat_int_t(Env *env, Value arg) {
    auto integer = arg->to_int(env);
    integer->assert_fixnum(env);
    return integer->integer().to_nat_int_t();
}

int IntegerObject::convert_to_int(Env *env, Value arg) {
    auto result = convert_to_nat_int_t(env, arg);

    if (result < std::numeric_limits<int>::min())
        env->raise("RangeError", "integer {} too small to convert to `int'");
    else if (result > std::numeric_limits<int>::max())
        env->raise("RangeError", "integer {} too big to convert to `int'");

    return (int)result;
}

}
