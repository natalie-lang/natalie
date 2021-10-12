#pragma once

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

    static bool optimized_method(SymbolValue *);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<IntegerValue %p int=%lli>", this, m_integer);
    }

private:
    inline static Hashmap<SymbolValue *> s_optimized_methods {};

    nat_int_t m_integer { 0 };
};

}
