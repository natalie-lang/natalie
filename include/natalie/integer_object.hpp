#pragma once

#include "natalie/big_int.hpp"
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

namespace Natalie {

class IntegerObject : public Object {
public:
    IntegerObject(nat_int_t integer)
        : Object { Object::Type::Integer, GlobalEnv::the()->Integer() }
        , m_integer { integer } { }

    IntegerObject(const Integer &integer)
        : Object { Object::Type::Integer, GlobalEnv::the()->Integer() }
        , m_integer { integer } { }

    static Value create(const Integer &);
    static Value create(const char *);

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

    static nat_int_t convert_to_nat_int_t(Env *, Value);
    static int convert_to_int(Env *, Value);

    static Value sqrt(Env *, Value);

    Value inspect(Env *env) { return to_s(env); }

    Value to_s(Env *, Value = nullptr);
    Value to_i();
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
    Value left_shift(Env *, Value);
    Value right_shift(Env *, Value);
    Value pred(Env *);
    Value size(Env *);
    Value succ(Env *);
    Value ceil(Env *, Value);
    Value coerce(Env *, Value);
    Value floor(Env *, Value);
    Value gcd(Env *, Value);
    Value abs(Env *);
    Value chr(Env *) const;
    Value negate(Env *);
    Value numerator();
    Value complement(Env *) const;
    Value ord() { return this; }
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
    bool has_to_be_bignum() const { return false; }
    bool is_fixnum() const { return !is_bignum(); }

    nat_int_t to_nat_int_t() { return integer().to_nat_int_t(); }
    BigInt to_bigint() const { return m_integer.to_bigint(); }

    void assert_fixnum(Env *env) const {
        if (is_bignum())
            env->raise("RangeError", "bignum too big to convert into `long'");
    }

    static bool optimized_method(SymbolObject *);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerObject %p value=%s is_fixnum=%s>", this, m_integer.to_string().c_str(), m_integer.is_fixnum() ? "true" : "false");
    }

private:
    inline static Hashmap<SymbolObject *> s_optimized_methods {};

    Integer m_integer;
};

}
