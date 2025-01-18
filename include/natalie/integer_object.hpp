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

class IntegerObject : public Object {
public:
    IntegerObject(const nat_int_t integer)
        : Object { Object::Type::Integer, GlobalEnv::the()->Integer() }
        , m_integer { integer } { }

    IntegerObject(const Integer &integer)
        : Object { Object::Type::Integer, GlobalEnv::the()->Integer() }
        , m_integer { integer } { }

    IntegerObject(Integer &&integer)
        : Object { Object::Type::Integer, GlobalEnv::the()->Integer() }
        , m_integer { std::move(integer) } { }

    static Value create(nat_int_t);
    static Value create(const Integer &);
    static Value create(Integer &&);
    static Value create(const char *);
    static Value create(const TM::String &);
    static Value create(TM::String &&);

    static Integer integer(IntegerObject *self) {
        return self->m_integer;
    }

    bool is_negative() const {
        return m_integer.is_negative();
    }

    static bool is_zero(IntegerObject *self) {
        return self->m_integer == 0;
    }

    static bool is_odd(IntegerObject *self) {
        return self->m_integer % 2 != 0;
    }

    static bool is_even(IntegerObject *self) {
        return !is_odd(self);
    }

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
        auto integer = arg->to_int(env);
        if (integer->is_bignum())
            env->raise("RangeError", "bignum too big to convert into '{}'", typeinfo<T>().name());
        const auto result = integer->to_nat_int_t();
        if (!std::numeric_limits<T>::is_signed && result < 0)
            env->raise("ArgumentError", "negative length {} given", result);
        if (result < static_cast<nat_int_t>(std::numeric_limits<T>::min()))
            env->raise("RangeError", "integer {} too small to convert to '{}'", result, typeinfo<T>().name());
        if (((std::numeric_limits<T>::is_signed && result > 0) || !std::numeric_limits<T>::is_signed) && static_cast<unsigned long long>(result) > std::numeric_limits<T>::max())
            env->raise("RangeError", "integer {} too big to convert to '{}'", result, typeinfo<T>().name());
        return static_cast<T>(result);
    }

    static Value sqrt(Env *, Value);

    static Value inspect(Env *env, IntegerObject *self) { return to_s(env, self); }

    String to_s() const { return m_integer.to_string(); }

    static Value to_s(Env *, IntegerObject *, Value = nullptr);
    static Value to_i(IntegerObject *);
    static Value to_f(IntegerObject *);
    static Value add(Env *, IntegerObject *, Value);
    static Value sub(Env *, IntegerObject *, Value);
    static Value mul(Env *, IntegerObject *, Value);
    static Value div(Env *, IntegerObject *, Value);
    static Value mod(Env *, IntegerObject *, Value);
    static Value pow(Env *, IntegerObject *, Value);
    static Value powmod(Env *, IntegerObject *, Value, Value);
    static Value cmp(Env *, IntegerObject *, Value);
    static Value times(Env *, IntegerObject *, Block *);
    static Value bitwise_and(Env *, IntegerObject *, Value);
    static Value bitwise_or(Env *, IntegerObject *, Value);
    static Value bitwise_xor(Env *, IntegerObject *, Value);
    static Value bit_length(Env *, IntegerObject *);
    static Value left_shift(Env *, IntegerObject *, Value);
    static Value right_shift(Env *, IntegerObject *, Value);
    static Value pred(Env *, IntegerObject *);
    static Value size(Env *, IntegerObject *);
    static Value succ(Env *, IntegerObject *);
    static Value ceil(Env *, IntegerObject *, Value);
    static Value coerce(Env *, IntegerObject *, Value);
    static Value floor(Env *, IntegerObject *, Value);
    static Value gcd(Env *, IntegerObject *, Value);
    static Value abs(Env *, IntegerObject *);
    static Value chr(Env *, IntegerObject *, Value);
    static Value negate(Env *, IntegerObject *);
    static Value numerator(IntegerObject *self) { return IntegerObject::create(self->m_integer); }
    static Value complement(Env *, IntegerObject *);
    static Value ord(IntegerObject *self) { return IntegerObject::create(self->m_integer); }
    static Value denominator() { return Value::integer(1); }
    static Value round(Env *, IntegerObject *, Value, Value);
    static Value truncate(Env *, IntegerObject *, Value);
    static Value ref(Env *, IntegerObject *, Value, Value);

    static bool neq(Env *, IntegerObject *, Value);
    static bool eq(Env *, IntegerObject *, Value);
    static bool lt(Env *, IntegerObject *, Value);
    static bool lte(Env *, IntegerObject *, Value);
    static bool gt(Env *, IntegerObject *, Value);
    static bool gte(Env *, IntegerObject *, Value);
    bool is_bignum() const { return m_integer.is_bignum(); }
    bool is_fixnum() const { return !is_bignum(); }

    nat_int_t to_nat_int_t() const { return m_integer.to_nat_int_t(); }
    BigInt to_bigint() const { return m_integer.to_bigint(); }

    void assert_fixnum(Env *env) const {
        if (is_bignum())
            env->raise("RangeError", "bignum too big to convert into 'long'");
    }

    virtual String dbg_inspect() const override { return to_s(); }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerObject %p value=%s is_fixnum=%s>", this, m_integer.to_string().c_str(), m_integer.is_fixnum() ? "true" : "false");
    }

private:
    Integer m_integer;
};

}
