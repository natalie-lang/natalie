#pragma once

#include "natalie/bigint.hpp"
#include <assert.h>
#include <inttypes.h>

#include "natalie/class_object.hpp"
#include "natalie/constants.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/integer.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/rounding_mode.hpp"
#include "natalie/types.hpp"
#include "nathelpers/typeinfo.hpp"

namespace Natalie {

class IntegerMethods {
public:
    IntegerMethods() = delete;

    static bool is_negative(const Integer self) { return self.is_negative(); }
    static bool is_zero(const Integer self) { return self.is_zero(); }
    static bool is_odd(const Integer self) { return self % 2 != 0; }
    static bool is_even(const Integer self) { return !is_odd(self); }

    static Value from_size_t(Env *env, size_t number) {
        assert(number <= NAT_INT_MAX);
        return Value::integer(static_cast<nat_int_t>(number));
    }

    static Value from_ssize_t(Env *env, ssize_t number) {
        assert(number <= NAT_INT_MAX && number >= NAT_INT_MIN);
        return Value::integer(static_cast<nat_int_t>(number));
    }

    static nat_int_t convert_to_nat_int_t(Env *, Value);
    static int convert_to_int(Env *, Value);
    static uid_t convert_to_uid(Env *, Value);
    static gid_t convert_to_gid(Env *, Value);

    template <class T>
    static T convert_to_native_type(Env *env, Value arg) {
        auto integer = arg.to_int(env);
        if (is_bignum(integer))
            env->raise("RangeError", "bignum too big to convert into '{}'", typeinfo<T>().name());
        const auto result = integer.to_nat_int_t();
        if (!std::numeric_limits<T>::is_signed && result < 0)
            env->raise("ArgumentError", "negative length {} given", result);
        if (result < static_cast<nat_int_t>(std::numeric_limits<T>::min()))
            env->raise("RangeError", "integer {} too small to convert to '{}'", result, typeinfo<T>().name());
        if (((std::numeric_limits<T>::is_signed && result > 0) || !std::numeric_limits<T>::is_signed) && static_cast<unsigned long long>(result) > std::numeric_limits<T>::max())
            env->raise("RangeError", "integer {} too big to convert to '{}'", result, typeinfo<T>().name());
        return static_cast<T>(result);
    }

    static Value sqrt(Env *, Value);

    static Value inspect(Env *env, Integer &self) { return to_s(env, self); }

    static String to_s(const Integer &self) { return self.to_string(); }

    static Value to_s(Env *, Integer &self, Value = nullptr);
    static Value to_i(Integer &self) { return self; }
    static Value to_f(Integer &self);
    static Value add(Env *, Integer &, Value);
    static Value sub(Env *, Integer &, Value);
    static Value mul(Env *, Integer &, Value);
    static Value div(Env *, Integer &, Value);
    static Value mod(Env *, Integer &, Value);
    static Value pow(Env *, Integer &, Integer &);
    static Value pow(Env *, Integer &, Value);
    static Value powmod(Env *, Integer &, Integer &, Integer &);
    static Value powmod(Env *, Integer &, Value, Value);
    static Value cmp(Env *, Integer &, Value);
    static Value times(Env *, Integer &, Block *);
    static Value bitwise_and(Env *, Integer &, Value);
    static Value bitwise_or(Env *, Integer &, Value);
    static Value bitwise_xor(Env *, Integer &, Value);
    static Value bit_length(Env *, Integer &self) { return self.bit_length(); }
    static Value left_shift(Env *, Integer &, Value);
    static Value right_shift(Env *, Integer &, Value);
    static Value pred(Env *env, Integer &self) { return self - 1; }
    static Value size(Env *, Integer &);
    static Value succ(Env *, Integer &self) { return self + 1; }
    static Value ceil(Env *, Integer &, Value);
    static Value coerce(Env *, Value, Value);
    static Value floor(Env *, Integer &, Value);
    static Value gcd(Env *, Integer &, Value);
    static Value abs(Env *, Integer &self) { return self.is_negative() ? -self : self; }
    static Value chr(Env *, Integer &, Value);
    static Value negate(Env *, Integer &self) { return -self; }
    static Value numerator(Integer &self) { return self; }
    static Value complement(Env *, Integer &self) { return ~self; }
    static Value ord(Integer &self) { return self; }
    static Value denominator() { return Value::integer(1); }
    static Value round(Env *, Integer &, Value, Value);
    static Value truncate(Env *, Integer &, Value);
    static Value ref(Env *, Integer &, Value, Value);

    static bool neq(Env *env, Value self, Value other) { return self.send(env, "=="_s, { other }).is_falsey(); }
    static bool eq(Env *, Integer &, Value);
    static bool lt(Env *, Integer &, Value);
    static bool lte(Env *, Integer &, Value);
    static bool gt(Env *, Integer &, Value);
    static bool gte(Env *, Integer &, Value);
    static bool is_bignum(const Integer &self) { return self.is_bignum(); }
    static bool is_fixnum(const Integer &self) { return self.is_fixnum(); }

    static BigInt to_bigint(const Integer &self) { return self.to_bigint(); }

    static void assert_fixnum(Env *env, const Integer &self) {
        if (self.is_bignum())
            env->raise("RangeError", "bignum too big to convert into 'long'");
    }
};

}
