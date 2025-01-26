#include "natalie.hpp"

#include <math.h>

namespace Natalie {

Value IntegerObject::create(const nat_int_t integer) {
    return Value::integer(integer);
}

Value IntegerObject::create(const Integer &integer) {
    return Value { integer };
}

Value IntegerObject::create(Integer &&integer) {
    return Value { std::move(integer) };
}

Value IntegerObject::create(const char *string) {
    String str { string };
    return create(str);
};

Value IntegerObject::create(const TM::String &string) {
    return Value { Integer(string) };
};

Value IntegerObject::create(TM::String &&string) {
    return Value { Integer(std::move(string)) };
};

Value IntegerObject::to_s(Env *env, Integer &self, Value base_value) {
    if (self == 0)
        return new StringObject { "0" };

    nat_int_t base = 10;
    if (base_value) {
        base = convert_to_nat_int_t(env, base_value);

        if (base < 2 || base > 36) {
            env->raise("ArgumentError", "invalid radix {}", base);
        }
    }

    if (base == 10)
        return new StringObject { self.to_string(), Encoding::US_ASCII };

    auto str = new StringObject { "", Encoding::US_ASCII };
    auto num = self;
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

Value IntegerObject::add(Env *env, Integer &self, Value arg) {
    if (arg.is_integer()) {
        return create(self + arg.integer());
    } else if (arg->is_float()) {
        return Value::floatingpoint(self + arg->as_float()->to_double());
    } else if (!arg.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        if (!lhs.is_integer())
            return lhs.send(env, "+"_s, { rhs });
        arg = rhs;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    return create(self + arg.integer());
}

Value IntegerObject::sub(Env *env, Integer &self, Value arg) {
    if (arg.is_integer()) {
        return create(self - arg.integer());
    } else if (arg->is_float()) {
        double result = self.to_double() - arg->as_float()->to_double();
        return Value::floatingpoint(result);
    } else if (!arg.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        if (!lhs.is_integer())
            return lhs.send(env, "-"_s, { rhs });
        arg = rhs;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    return create(self - arg.integer());
}

Value IntegerObject::mul(Env *env, Integer &self, Value arg) {
    if (arg->is_float()) {
        double result = self.to_double() * arg->as_float()->to_double();
        return new FloatObject { result };
    } else if (!arg.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        if (!lhs.is_integer())
            return lhs.send(env, "*"_s, { rhs });
        arg = rhs;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    if (self == 0 || arg.integer() == 0)
        return Value::integer(0);

    return create(self * arg.integer());
}

Value IntegerObject::div(Env *env, Integer &self, Value arg) {
    if (arg->is_float()) {
        double result = self / arg->as_float()->to_double();
        if (isnan(result))
            return FloatObject::nan();
        return Value::floatingpoint(result);
    } else if (!arg.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        if (!lhs.is_integer())
            return lhs.send(env, "/"_s, { rhs });
        arg = rhs;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto other = arg.integer();
    if (other == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    return create(self / other);
}

Value IntegerObject::mod(Env *env, Integer &self, Value arg) {
    Integer argument;
    if (arg->is_float()) {
        return Value::floatingpoint(self.to_double())->as_float()->mod(env, arg);
    } else if (!arg.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        if (!lhs.is_integer())
            return lhs.send(env, "%"_s, { rhs });
        arg = rhs;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");
    argument = arg->as_integer()->m_integer;

    if (argument == 0)
        env->raise("ZeroDivisionError", "divided by 0");

    return create(self % argument);
}

Value IntegerObject::pow(Env *env, Integer &self, Integer &arg) {
    if (self == 0 && arg < 0)
        env->raise("ZeroDivisionError", "divided by 0");

    // NATFIXME: If a negative number is passed we want to return a Rational
    if (arg < 0) {
        auto denominator = Natalie::pow(self, -arg);
        return new RationalObject { Value::integer(1), denominator };
    }

    if (arg == 0)
        return Value::integer(1);
    else if (arg == 1)
        return create(self);

    if (self == 0)
        return Value::integer(0);
    else if (self == 1)
        return Value::integer(1);
    else if (self == -1)
        return Value::integer(arg % 2 ? 1 : -1);

    // NATFIXME: Ruby has a really weird max bignum value that is calculated by the words needed to store a bignum
    // The calculation that we do is pretty much guessed to be in the direction of ruby. However, we should do more research about this limit...
    size_t length = self.to_string().length();
    constexpr const size_t BIGINT_LIMIT = 8 * 1024 * 1024;
    if (length > BIGINT_LIMIT || length * arg > (nat_int_t)BIGINT_LIMIT)
        env->raise("ArgumentError", "exponent is too large");

    return create(Natalie::pow(self, arg));
}

Value IntegerObject::pow(Env *env, Integer &self, Value arg) {
    if (arg.is_fast_integer())
        return pow(env, self, arg.integer());

    if ((arg->is_float() || arg->is_rational()) && self < 0) {
        auto comp = new ComplexObject { self };
        return comp->send(env, "**"_s, { arg });
    }

    if (arg->is_float())
        return Value::floatingpoint(self.to_double())->as_float()->pow(env, arg);

    if (!arg.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        if (!lhs.is_integer())
            return lhs.send(env, "**"_s, { rhs });
        arg = rhs;
    }

    arg->assert_type(env, Object::Type::Integer, "Integer");

    return pow(env, self, arg->as_integer()->m_integer);
}

Value IntegerObject::powmod(Env *env, Integer &self, Value exponent, Value mod) {
    if (exponent.is_integer() && IntegerObject::is_negative(exponent->as_integer()) && mod)
        env->raise("RangeError", "2nd argument not allowed when first argument is negative");

    auto powd = pow(env, self, exponent);

    if (!mod)
        return powd;

    if (!mod.is_integer())
        env->raise("TypeError", "2nd argument not allowed unless all arguments are integers");

    auto modi = mod->as_integer();
    if (to_nat_int_t(modi) == 0)
        env->raise("ZeroDivisionError", "cannot divide by zero");

    auto powi = powd->as_integer();

    if (is_bignum(powi))
        return Integer(to_bigint(powi) % to_bigint(modi));

    if (to_nat_int_t(powi) < 0 || to_nat_int_t(modi) < 0)
        return IntegerObject::mod(env, IntegerObject::integer(powi), mod);

    return Value::integer(to_nat_int_t(powi) % to_nat_int_t(modi));
}

Value IntegerObject::cmp(Env *env, Integer &self, Value arg) {
    auto is_comparable_with = [](Value arg) -> bool {
        return arg.is_fast_integer() || arg.is_integer() || (arg->is_float() && !arg->as_float()->is_nan());
    };

    // Check if we might want to coerce the value
    if (!is_comparable_with(arg)) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self, Natalie::CoerceInvalidReturnValueMode::Allow);
        if (!is_comparable_with(lhs))
            return lhs.send(env, "<=>"_s, { rhs });
        arg = rhs;
    }

    // Check if comparable
    if (!is_comparable_with(arg))
        return NilObject::the();

    if (lt(env, self, arg)) {
        return Value::integer(-1);
    } else if (IntegerObject::eq(env, self, arg)) {
        return Value::integer(0);
    } else {
        return Value::integer(1);
    }
}

bool IntegerObject::neq(Env *env, IntegerObject *self, Value other) {
    return self->send(env, "=="_s, { other })->is_falsey();
}

bool IntegerObject::eq(Env *env, Integer &self, Value other) {
    if (other.is_fast_integer())
        return self == other.integer();

    if (other->is_float()) {
        auto *f = other->as_float();
        return !f->is_nan() && self == f->to_double();
    }

    if (!other.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, other, self);
        if (!lhs.is_integer())
            return lhs->send(env, "=="_s, { rhs })->is_truthy();
        other = rhs;
    }

    if (other.is_integer())
        return self == other->as_integer()->m_integer;

    return other->send(env, "=="_s, { self })->is_truthy();
}

bool IntegerObject::lt(Env *env, IntegerObject *self, Value other) {
    if (other->is_float()) {
        if (other->as_float()->is_nan())
            return false;
        return self->m_integer < other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, other, self);
        if (!lhs.is_integer())
            return lhs->send(env, "<"_s, { rhs })->is_truthy();
        other = rhs;
    }

    if (other.is_integer())
        return self->m_integer < other->as_integer()->m_integer;

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, self);
        return result.first->send(env, "<"_s, { result.second })->is_truthy();
    }

    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::lte(Env *env, IntegerObject *self, Value other) {
    if (other->is_float()) {
        if (other->as_float()->is_nan())
            return false;
        return self->m_integer <= other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, other, self);
        if (!lhs.is_integer())
            return lhs->send(env, "<="_s, { rhs })->is_truthy();
        other = rhs;
    }

    if (other.is_integer())
        return self->m_integer <= other->as_integer()->m_integer;

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, self);
        return result.first->send(env, "<="_s, { result.second })->is_truthy();
    }

    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::gt(Env *env, IntegerObject *self, Value other) {
    if (other->is_float()) {
        if (other->as_float()->is_nan())
            return false;
        return self->m_integer > other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, other, self, Natalie::CoerceInvalidReturnValueMode::Raise);
        if (!lhs.is_integer())
            return lhs->send(env, ">"_s, { rhs })->is_truthy();
        other = rhs;
    }

    if (other.is_integer())
        return self->m_integer > other->as_integer()->m_integer;

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, self);
        return result.first->send(env, ">"_s, { result.second })->is_truthy();
    }

    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

bool IntegerObject::gte(Env *env, IntegerObject *self, Value other) {
    if (other->is_float()) {
        if (other->as_float()->is_nan())
            return false;
        return self->m_integer >= other->as_float()->to_double();
    }

    if (!other.is_integer()) {
        auto [lhs, rhs] = Natalie::coerce(env, other, self, Natalie::CoerceInvalidReturnValueMode::Raise);
        if (!lhs.is_integer())
            return lhs->send(env, ">="_s, { rhs })->is_truthy();
        other = rhs;
    }

    if (other.is_integer())
        return self->m_integer >= other->as_integer()->m_integer;

    if (other->respond_to(env, "coerce"_s)) {
        auto result = Natalie::coerce(env, other, self);
        return result.first->send(env, ">="_s, { result.second })->is_truthy();
    }

    env->raise("ArgumentError", "comparison of Integer with {} failed", other->inspect_str(env));
}

Value IntegerObject::times(Env *env, Integer &self, Block *block) {
    if (!block) {
        auto enumerator = Value(self).send(env, "enum_for"_s, { "times"_s });
        enumerator->ivar_set(env, "@size"_s, self < 0 ? Value::integer(0) : self);
        return enumerator;
    }

    if (self <= 0)
        return self;

    for (Integer i = 0; i < self; ++i) {
        Value num = create(i);
        Value args[] = { num };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    }
    return self;
}

Value IntegerObject::bitwise_and(Env *env, Integer &self, Value arg) {
    if (!arg.is_integer() && arg->respond_to(env, "coerce"_s)) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        auto and_symbol = "&"_s;
        if (!lhs.is_integer() && lhs->respond_to(env, and_symbol))
            return lhs.send(env, and_symbol, { rhs });
        arg = rhs;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    return create(self & arg.integer());
}

Value IntegerObject::bitwise_or(Env *env, Integer &self, Value arg) {
    Integer argument;
    if (!arg.is_integer() && arg->respond_to(env, "coerce"_s)) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        auto or_symbol = "|"_s;
        if (!lhs.is_integer() && lhs->respond_to(env, or_symbol))
            return lhs.send(env, or_symbol, { rhs });
        arg = rhs;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    return create(self | arg.integer());
}

Value IntegerObject::bitwise_xor(Env *env, Integer &self, Value arg) {
    Integer argument;
    if (!arg.is_integer() && arg->respond_to(env, "coerce"_s)) {
        auto [lhs, rhs] = Natalie::coerce(env, arg, self);
        auto xor_symbol = "^"_s;
        if (!lhs.is_integer() && lhs->respond_to(env, xor_symbol))
            return lhs.send(env, xor_symbol, { rhs });
        arg = rhs;
    }
    arg->assert_type(env, Object::Type::Integer, "Integer");

    return create(self ^ arg.integer());
}

Value IntegerObject::left_shift(Env *env, Integer &self, Value arg) {
    auto integer = Object::to_int(env, arg);
    if (is_bignum(integer)) {
        if (IntegerObject::is_negative(self))
            return Value::integer(-1);
        else
            return Value::integer(0);
    }

    auto nat_int = integer.to_nat_int_t();

    if (nat_int < 0)
        return IntegerObject::right_shift(env, self, Value::integer(-nat_int));

    return create(self << nat_int);
}

Value IntegerObject::right_shift(Env *env, Integer &self, Value arg) {
    auto integer = Object::to_int(env, arg);
    if (is_bignum(integer)) {
        if (IntegerObject::is_negative(self))
            return Value::integer(-1);
        else
            return Value::integer(0);
    }

    auto nat_int = integer.to_nat_int_t();

    if (nat_int < 0)
        return left_shift(env, self, Value::integer(-nat_int));

    return create(self >> nat_int);
}

Value IntegerObject::size(Env *env, Integer &self) {
    if (is_bignum(self)) {
        const nat_int_t bitstring_size = IntegerObject::to_s(env, self, Value::integer(2))->as_string()->bytesize();
        return Value::integer((bitstring_size + 7) / 8);
    }
    return Value::integer(sizeof(nat_int_t));
}

Value IntegerObject::succ(Env *env, IntegerObject *self) {
    return add(env, IntegerObject::integer(self), Value::integer(1));
}

Value IntegerObject::coerce(Env *env, IntegerObject *self, Value arg) {
    ArrayObject *ary = new ArrayObject {};
    switch (arg->type()) {
    case Object::Type::Integer:
        ary->push(arg);
        ary->push(self);
        break;
    case Object::Type::String:
        ary->push(self->send(env, "Float"_s, { arg }));
        ary->push(self->send(env, "to_f"_s));
        break;
    default:
        if (!arg->is_nil() && !arg->is_float() && arg->respond_to(env, "to_f"_s)) {
            arg = arg.send(env, "to_f"_s);
        }

        if (arg->is_float()) {
            ary->push(arg);
            ary->push(self->send(env, "to_f"_s));
            break;
        }

        env->raise("TypeError", "can't convert {} into Float", arg->inspect_str(env));
    }
    return ary;
}

Value IntegerObject::ceil(Env *env, IntegerObject *self, Value arg) {
    if (arg == nullptr)
        return IntegerObject::create(self->m_integer);

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto precision = arg->as_integer()->m_integer.to_nat_int_t();
    if (precision >= 0)
        return IntegerObject::create(self->m_integer);

    double f = ::pow(10, precision);
    auto result = ::ceil(self->m_integer.to_nat_int_t() * f) / f;

    return Value::integer(result);
}

Value IntegerObject::floor(Env *env, IntegerObject *self, Value arg) {
    if (arg == nullptr)
        return IntegerObject::create(self->m_integer);

    arg->assert_type(env, Object::Type::Integer, "Integer");

    auto precision = arg->as_integer()->m_integer.to_nat_int_t();
    if (precision >= 0)
        return IntegerObject::create(self->m_integer);

    double f = ::pow(10, precision);
    auto result = ::floor(self->m_integer.to_nat_int_t() * f) / f;

    return Value::integer(result);
}

Value IntegerObject::gcd(Env *env, IntegerObject *self, Value divisor) {
    divisor->assert_type(env, Object::Type::Integer, "Integer");
    return create(Natalie::gcd(self->m_integer, divisor->as_integer()->m_integer));
}

Value IntegerObject::abs(Env *env, IntegerObject *self) {
    if (self->m_integer.is_negative())
        return create(-self->m_integer);
    return IntegerObject::create(self->m_integer);
}

Value IntegerObject::chr(Env *env, IntegerObject *self, Value encoding) {
    if (self->m_integer < 0 || self->m_integer > (nat_int_t)UINT_MAX)
        env->raise("RangeError", "{} out of char range", self->m_integer.to_string());
    else if (is_bignum(self))
        env->raise("RangeError", "bignum out of char range");

    if (encoding) {
        if (!encoding->is_encoding()) {
            encoding->assert_type(env, Type::String, "String");
            encoding = EncodingObject::find(env, encoding);
        }
    } else if (self->m_integer <= 127) {
        encoding = EncodingObject::get(Encoding::US_ASCII);
    } else if (self->m_integer < 256) {
        encoding = EncodingObject::get(Encoding::ASCII_8BIT);
    } else if (EncodingObject::default_internal()) {
        encoding = EncodingObject::default_internal();
    } else {
        env->raise("RangeError", "{} out of char range", self->m_integer.to_string());
    }

    auto encoding_obj = encoding->as_encoding();
    if (!encoding_obj->in_encoding_codepoint_range(self->m_integer.to_nat_int_t()))
        env->raise("RangeError", "{} out of char range", self->m_integer.to_string());

    if (!encoding_obj->valid_codepoint(self->m_integer.to_nat_int_t())) {
        auto hex = String();
        hex.append_sprintf("0x%X", self->m_integer.to_nat_int_t());

        auto encoding_name = encoding_obj->name()->as_string()->string();
        env->raise("RangeError", "invalid codepoint {} in {}", hex, encoding_name);
    }

    auto encoded = encoding_obj->encode_codepoint(self->m_integer.to_nat_int_t());
    return new StringObject { encoded, encoding_obj };
}

Value IntegerObject::negate(Env *env, IntegerObject *self) {
    return create(-self->m_integer);
}

Value IntegerObject::complement(Env *env, IntegerObject *self) {
    return create(~self->m_integer);
}

Value IntegerObject::sqrt(Env *env, Value arg) {
    auto argument = Object::to_int(env, arg);

    if (argument < 0) {
        auto domain_error = fetch_nested_const({ "Math"_s, "DomainError"_s });
        auto message = new StringObject { "Numerical argument is out of domain - \"isqrt\"" };
        auto exception = new ExceptionObject { domain_error->as_class(), message };
        env->raise_exception(exception);
    }

    return create(Natalie::sqrt(argument));
}

Value IntegerObject::round(Env *env, IntegerObject *self, Value ndigits, Value half) {
    if (!ndigits)
        return IntegerObject::create(self->m_integer);

    int digits = IntegerObject::convert_to_int(env, ndigits);
    RoundingMode rounding_mode = rounding_mode_from_value(env, half);

    if (digits >= 0)
        return IntegerObject::create(self->m_integer);

    auto result = self->m_integer;
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

Value IntegerObject::truncate(Env *env, IntegerObject *self, Value ndigits) {
    if (!ndigits)
        return IntegerObject::create(self->m_integer);

    int digits = IntegerObject::convert_to_int(env, ndigits);

    if (digits >= 0)
        return IntegerObject::create(self->m_integer);

    auto result = self->m_integer;
    auto dividend = Natalie::pow(Integer(10), -digits);
    auto remainder = result.modulo_c(dividend);

    return create(result - remainder);
}

Value IntegerObject::ref(Env *env, IntegerObject *self, Value offset_obj, Value size_obj) {
    auto from_offset_and_size = [self, env](Optional<nat_int_t> offset_or_empty, Optional<nat_int_t> size_or_empty = {}) -> Value {
        auto offset = offset_or_empty.value_or(0);

        if (!size_or_empty.present() && offset < 0)
            return Value::integer(0);

        auto size = size_or_empty.value_or(1);

        Integer result;
        if (offset < 0)
            result = self->m_integer << -offset;
        else
            result = self->m_integer >> offset;

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
            auto begin_obj = Object::to_int(env, range->begin());
            begin = begin_obj.to_nat_int_t();
        }

        Optional<nat_int_t> end;
        if (!range->end()->is_nil()) {
            auto end_obj = Object::to_int(env, range->end());
            end = end_obj.to_nat_int_t();
        }

        Optional<nat_int_t> size;
        if (!end || (begin && end.value() < begin.value()))
            size = -1;
        else if (end)
            size = end.value() - begin.value_or(0) + 1;

        return from_offset_and_size(begin, size);
    } else {
        auto offset_integer = Object::to_int(env, offset_obj);
        if (is_bignum(offset_integer))
            return Value::integer(0);

        auto offset = offset_integer.to_nat_int_t();

        Optional<nat_int_t> size;
        if (size_obj) {
            auto size_integer = Object::to_int(env, size_obj);
            if (is_bignum(size_integer))
                env->raise("RangeError", "shift width too big");

            size = size_integer.to_nat_int_t();
        }

        return from_offset_and_size(offset, size);
    }
}

nat_int_t IntegerObject::convert_to_nat_int_t(Env *env, Value arg) {
    auto integer = Object::to_int(env, arg);
    assert_fixnum(env, integer);
    return integer.to_nat_int_t();
}

int IntegerObject::convert_to_int(Env *env, Value arg) {
    auto result = convert_to_nat_int_t(env, arg);

    if (result < std::numeric_limits<int>::min())
        env->raise("RangeError", "integer {} too small to convert to 'int'");
    else if (result > std::numeric_limits<int>::max())
        env->raise("RangeError", "integer {} too big to convert to 'int'");

    return (int)result;
}

gid_t IntegerObject::convert_to_gid(Env *env, Value arg) {
    if (arg->is_nil()) return (gid_t)(-1); // special case for nil
    auto result = convert_to_nat_int_t(env, arg);
    // this lower limit may look incorrect but experimentally matches MRI behavior
    if (result < std::numeric_limits<int>::min())
        env->raise("RangeError", "integer {} too small to convert to 'unsigned int'", result);
    else if (result > std::numeric_limits<unsigned int>::max())
        env->raise("RangeError", "integer {} too big to convert to 'unsigned int'", result);
    return (gid_t)result;
}

uid_t IntegerObject::convert_to_uid(Env *env, Value arg) {
    if (arg->is_nil()) return (uid_t)(-1); // special case for nil
    auto result = convert_to_nat_int_t(env, arg);
    // this lower limit may look incorrect but experimentally matches MRI behavior
    if (result < std::numeric_limits<int>::min())
        env->raise("RangeError", "integer {} too small to convert to 'unsigned int'", result);
    else if (result > std::numeric_limits<unsigned int>::max())
        env->raise("RangeError", "integer {} too big to convert to 'unsigned int'", result);
    return (uid_t)result;
}

}
