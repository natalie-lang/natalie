#pragma once

#include <assert.h>
#include <inttypes.h>

#include "natalie/bignum/bignum.hpp"
#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/string_value.hpp"
#include "natalie/types.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class IntegerValue : public Value {
public:
    IntegerValue(nat_int_t integer)
        : Value { Value::Type::Integer, GlobalEnv::the()->Integer() }
        , m_integer { integer } { }

    IntegerValue(const char *bignum_str)
        : Value { Value::Type::Integer, GlobalEnv::the()->Integer() }
        , m_is_bignum(true)
        // TODO: make bignum use natalies strings?
        , m_bignum(bignum(std::string(bignum_str))) { }

    IntegerValue(bignum &bignum)
        : Value { Value::Type::Integer, GlobalEnv::the()->Integer() }
        , m_is_bignum(true)
        , m_bignum(std::move(bignum)) { }

    nat_int_t to_nat_int_t() {
        return m_integer;
    }

    bool is_zero() {
        return m_integer == 0;
    }

    bool is_odd() {
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

    ValuePtr to_s(Env *, ValuePtr = nullptr);
    ValuePtr to_i();
    ValuePtr to_f();
    ValuePtr add(Env *, ValuePtr);
    ValuePtr sub(Env *, ValuePtr);
    ValuePtr mul(Env *, ValuePtr);
    ValuePtr div(Env *, ValuePtr);
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

    bool eq(Env *, ValuePtr);
    bool eql(Env *, ValuePtr);
    bool lt(Env *, ValuePtr);
    bool lte(Env *, ValuePtr);
    bool gt(Env *, ValuePtr);
    bool gte(Env *, ValuePtr);
    bool is_bignum() const { return m_is_bignum; }
    bool is_fixnum() const { return !is_bignum(); }

    bignum to_bignum(Env *env) {
        if (is_fixnum()) {
            return bignum(to_s(env)->as_string()->c_str());
        }
        return m_bignum;
    }
    void set_bignum(bignum &bignum) {
        m_bignum = bignum;
        m_is_bignum = true;
    }

    static bool optimized_method(SymbolValue *);

    virtual void gc_inspect(char *buf, size_t len) const override {
        if (is_fixnum()) {
            snprintf(buf, len, "<IntegerValue %p int=%lli>", this, m_integer);
        } else {
            snprintf(buf, len, "<IntegerValue %p bignum=%s>", this, m_bignum.to_string().c_str());
        }
    }

private:
    inline static Hashmap<SymbolValue *> s_optimized_methods {};

    nat_int_t m_integer { 0 };
    bool m_is_bignum = false;
    bignum m_bignum;
};

}
