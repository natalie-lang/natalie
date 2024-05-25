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

    Integer integer() const {
        return m_integer;
    }

    bool is_negative() const {
        return m_integer.is_negative();
    }

    bool is_zero() const {
        return m_integer == 0;
    }

    bool is_odd() const {
        return m_integer % 2 != 0;
    }

    bool is_even() const {
        return !is_odd();
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
            env->raise("RangeError", "bignum too big to convert into `{}'", typeinfo<T>().name());
        const auto result = integer->to_nat_int_t();
        if (!std::numeric_limits<T>::is_signed && result < 0)
            env->raise("ArgumentError", "negative length {} given", result);
        if (result < static_cast<nat_int_t>(std::numeric_limits<T>::min()))
            env->raise("RangeError", "integer {} too small to convert to `{}'", result, typeinfo<T>().name());
        if (static_cast<unsigned long long>(result) > std::numeric_limits<T>::max())
            env->raise("RangeError", "integer {} too big to convert to `{}' (max = {})", result, typeinfo<T>().name());
        return static_cast<T>(result);
    }

    static Value sqrt(Env *, Value);

    Value inspect(Env *env) const { return to_s(env); }

    String to_s() const { return m_integer.to_string(); }

    Value to_s(Env *, Value = nullptr) const;
    Value to_i() const;
    Value to_f() const;
    Value add(Env *, Value);
    Value sub(Env *, Value);
    Value mul(Env *, Value);
    Value div(Env *, Value);
    Value mod(Env *, Value);
    Value pow(Env *, Value);
    Value powmod(Env *, Value, Value);
    Value cmp(Env *, Value);
    Value times(Env *, Block *);
    Value bitwise_and(Env *, Value);
    Value bitwise_or(Env *, Value);
    Value bitwise_xor(Env *, Value);
    Value bit_length(Env *);
    Value left_shift(Env *, Value);
    Value right_shift(Env *, Value);
    Value pred(Env *);
    Value size(Env *) const;
    Value succ(Env *);
    Value ceil(Env *, Value);
    Value coerce(Env *, Value);
    Value floor(Env *, Value);
    Value gcd(Env *, Value);
    Value abs(Env *);
    Value chr(Env *, Value) const;
    Value negate(Env *);
    Value numerator();
    Value complement(Env *) const;
    Value ord() { return IntegerObject::create(m_integer); }
    Value denominator() { return Value::integer(1); }
    Value round(Env *, Value, Value);
    Value truncate(Env *, Value);
    Value ref(Env *, Value, Value);

    bool eq(Env *, Value);
    bool lt(Env *, Value);
    bool lte(Env *, Value);
    bool gt(Env *, Value);
    bool gte(Env *, Value);
    bool is_bignum() const { return m_integer.is_bignum(); }
    bool is_fixnum() const { return !is_bignum(); }

    nat_int_t to_nat_int_t() const { return integer().to_nat_int_t(); }
    BigInt to_bigint() const { return m_integer.to_bigint(); }

    void assert_fixnum(Env *env) const {
        if (is_bignum())
            env->raise("RangeError", "bignum too big to convert into `long'");
    }

    virtual String dbg_inspect() const override { return to_s(); }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerObject %p value=%s is_fixnum=%s>", this, m_integer.to_string().c_str(), m_integer.is_fixnum() ? "true" : "false");
    }

private:
    Integer m_integer;
};

}
