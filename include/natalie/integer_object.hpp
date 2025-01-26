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

    static Integer integer(const IntegerObject *self) {
        return self->m_integer;
    }

    static Integer &integer(IntegerObject *self) {
        return self->m_integer;
    }

    static bool is_negative(const IntegerObject *self) { return self->m_integer.is_negative(); }
    static bool is_negative(const Integer self) { return self.is_negative(); }

    static bool is_zero(const IntegerObject *self) { return is_zero(self->m_integer); }
    static bool is_zero(const Integer self) { return self == 0; }

    static bool is_odd(const IntegerObject *self) { return is_odd(self->m_integer); }
    static bool is_odd(const Integer self) { return self % 2 != 0; }

    static bool is_even(const IntegerObject *self) { return !is_odd(self); }
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
        auto integer = Object::to_int(env, arg);
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

    static Value inspect(Env *env, IntegerObject *self) { return to_s(env, self->m_integer); }
    static Value inspect(Env *env, Integer &self) { return to_s(env, self); }

    static String to_s(const IntegerObject *self) { return self->m_integer.to_string(); }
    static String to_s(const Integer &self) { return self.to_string(); }

    static Value to_s(Env *, Integer &self, Value = nullptr);
    static Value to_i(Integer &self) { return self; }
    static Value to_f(Integer &self) { return Value::floatingpoint(self.to_double()); }
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

    static bool neq(Env *env, Value self, Value other) { return self.send(env, "=="_s, { other })->is_falsey(); }
    static bool eq(Env *, Integer &, Value);
    static bool lt(Env *, Integer &, Value);
    static bool lte(Env *, Integer &, Value);
    static bool gt(Env *, Integer &, Value);
    static bool gte(Env *, IntegerObject *, Value);
    static bool gte(Env *env, Integer &self, Value other) { return gte(env, new IntegerObject(self), other); }
    static bool is_bignum(const IntegerObject *self) { return self->m_integer.is_bignum(); }
    static bool is_bignum(const Integer &self) { return self.is_bignum(); }
    static bool is_fixnum(const IntegerObject *self) { return !is_bignum(self); }
    static bool is_fixnum(const Integer &self) { return self.is_fixnum(); }

    static nat_int_t to_nat_int_t(const IntegerObject *self) { return self->m_integer.to_nat_int_t(); }
    static BigInt to_bigint(const IntegerObject *self) { return self->m_integer.to_bigint(); }
    static BigInt to_bigint(const Integer &self) { return self.to_bigint(); }

    static void assert_fixnum(Env *env, const IntegerObject *self) {
        if (is_bignum(self))
            env->raise("RangeError", "bignum too big to convert into 'long'");
    }

    static void assert_fixnum(Env *env, const Integer &self) {
        if (self.is_bignum())
            env->raise("RangeError", "bignum too big to convert into 'long'");
    }

    virtual String dbg_inspect() const override { return to_s(this); }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerObject %p value=%s is_fixnum=%s>", this, m_integer.to_string().c_str(), m_integer.is_fixnum() ? "true" : "false");
    }

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        if (m_integer.is_bignum())
            visitor.visit(m_integer.bigint_pointer());
    }

private:
    Integer m_integer;
};

}
