#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/types.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct IntegerValue : Value {
    IntegerValue(Env *env, nat_int_t integer)
        : Value { Value::Type::Integer, env->Integer() }
        , m_integer { integer } { }

    IntegerValue(GlobalEnv *global_env, nat_int_t integer)
        : Value { Value::Type::Integer, global_env->Integer() }
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
        return ValuePtr { env, static_cast<nat_int_t>(number) };
    }

    ValuePtr inspect(Env *env) { return to_s(env); }

    ValuePtr to_s(Env *, ValuePtr = nullptr);
    ValuePtr to_i();
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

    bool eq(Env *, ValuePtr);
    bool eql(Env *, ValuePtr);
    bool lt(Env *, ValuePtr);
    bool lte(Env *, ValuePtr);
    bool gt(Env *, ValuePtr);
    bool gte(Env *, ValuePtr);

    static bool optimized_method(SymbolValue *);

    virtual char *gc_repr() override {
        char *buf = new char[100];
        snprintf(buf, 100, "<IntegerValue %p int=%li>", this, m_integer);
        return buf;
    }

private:
    nat_int_t m_integer { 0 };
};

}
