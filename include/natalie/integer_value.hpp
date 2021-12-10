#pragma once

#include "natalie/big_int.hpp"
#include <assert.h>
#include <inttypes.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/types.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class IntegerValue : public Value {
public:
    IntegerValue(nat_int_t integer)
        : Value { Value::Type::Integer, GlobalEnv::the()->Integer() }
        , m_integer { integer } { }

    nat_int_t to_nat_int_t() const {
        return m_integer;
    }

    bool is_zero() {
        return m_integer == 0;
    }

    virtual bool is_odd() {
        return m_integer % 2 != 0;
    }

    bool is_even() {
        return m_integer % 2 == 0;
    }

    static ValuePtr from_size_t(Env *env, size_t number) {
        assert(number <= NAT_INT_MAX);
        return ValuePtr::integer(static_cast<nat_int_t>(number));
    }

    ValuePtr inspect(Env *env) { return to_s(env); }

    virtual ValuePtr to_s(Env *, ValuePtr = nullptr);
    ValuePtr to_i();
    ValuePtr to_f();
    virtual ValuePtr add(Env *, ValuePtr);
    virtual ValuePtr sub(Env *, ValuePtr);
    virtual ValuePtr mul(Env *, ValuePtr);
    virtual ValuePtr div(Env *, ValuePtr);
    ValuePtr mod(Env *, ValuePtr);
    ValuePtr pow(Env *, ValuePtr);
    ValuePtr cmp(Env *, ValuePtr);
    ValuePtr eqeqeq(Env *, ValuePtr);
    ValuePtr times(Env *, Block *);
    ValuePtr bitwise_and(Env *, ValuePtr);
    ValuePtr bitwise_or(Env *, ValuePtr);
    ValuePtr succ(Env *);
    ValuePtr coerce(Env *, ValuePtr);
    ValuePtr abs(Env *);
    ValuePtr chr(Env *);
    ValuePtr negate(Env *);
    ValuePtr complement(Env *);
    ValuePtr ord() { return this; }

    virtual bool eq(Env *, ValuePtr);
    virtual bool eql(Env *, ValuePtr);
    virtual bool lt(Env *, ValuePtr);
    virtual bool lte(Env *, ValuePtr);
    virtual bool gt(Env *, ValuePtr);
    virtual bool gte(Env *, ValuePtr);
    virtual bool is_bignum() const { return false; }
    bool is_fixnum() const { return !is_bignum(); }

    void assert_fixnum(Env *env) {
        if (is_bignum())
            env->raise("RangeError", "bignum too big to convert into `long'");
    }

    virtual BigInt to_bignum() const { return BigInt(to_nat_int_t()); }

    static bool optimized_method(SymbolValue *);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerValue %p int=%lli>", this, m_integer);
    }

private:
    inline static Hashmap<SymbolValue *> s_optimized_methods {};

    nat_int_t m_integer { 0 };

    nat_int_t div_floor(nat_int_t);
};

}
