#pragma once

#include "natalie/big_int.hpp"
#include <assert.h>
#include <inttypes.h>

#include "natalie/class_object.hpp"
#include "natalie/constants.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/types.hpp"

namespace Natalie {

class IntegerObject : public Object {
public:
    IntegerObject(nat_int_t integer)
        : Object { Object::Type::Integer, GlobalEnv::the()->Integer() }
        , m_integer { integer } { }

    virtual nat_int_t to_nat_int_t() const {
        return m_integer;
    }

    virtual bool is_negative() const {
        return m_integer < 0;
    }

    bool is_zero() const {
        return m_integer == 0;
    }

    virtual bool is_odd() const {
        return m_integer % 2 != 0;
    }

    bool is_even() const {
        return !is_odd();
    }

    static Value from_size_t(Env *env, size_t number) {
        assert(number <= NAT_INT_MAX);
        return Value::integer(static_cast<nat_int_t>(number));
    }

    Value inspect(Env *env) { return to_s(env); }

    virtual Value to_s(Env *, Value = nullptr);
    Value to_i();
    virtual Value to_f() const;
    virtual Value add(Env *, Value);
    virtual Value sub(Env *, Value);
    virtual Value mul(Env *, Value);
    virtual Value div(Env *, Value);
    virtual Value mod(Env *, Value);
    virtual Value pow(Env *, Value);
    Value cmp(Env *, Value);
    virtual Value times(Env *, Block *);
    Value bitwise_and(Env *, Value) const;
    Value bitwise_or(Env *, Value) const;
    Value pred(Env *);
    Value size(Env *);
    Value succ(Env *);
    Value ceil(Env *, Value);
    Value coerce(Env *, Value);
    Value floor(Env *, Value);
    virtual Value gcd(Env *, Value);
    virtual Value abs(Env *);
    Value chr(Env *) const;
    virtual Value negate(Env *);
    Value numerator();
    Value complement(Env *) const;
    Value ord() { return this; }
    Value denominator() { return Value::integer(1); }

    virtual bool eq(Env *, Value);
    virtual bool eql(Env *, Value);
    virtual bool lt(Env *, Value);
    virtual bool lte(Env *, Value);
    virtual bool gt(Env *, Value);
    virtual bool gte(Env *, Value);
    virtual bool is_bignum() const { return false; }
    virtual bool has_to_be_bignum() const { return false; }
    bool is_fixnum() const { return !is_bignum(); }

    void assert_fixnum(Env *env) const {
        if (is_bignum())
            env->raise("RangeError", "bignum too big to convert into `long'");
    }

    virtual BigInt to_bigint() const { return BigInt(to_nat_int_t()); }

    static bool optimized_method(SymbolObject *);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerObject %p int=%lli>", this, m_integer);
    }

private:
    inline static Hashmap<SymbolObject *> s_optimized_methods {};

    nat_int_t m_integer { 0 };
};

}
