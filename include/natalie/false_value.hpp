#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct FalseValue : Value {
    FalseValue(Env *env)
        : Value { Value::Type::False, env->Object()->const_fetch(env, SymbolValue::intern(env, "FalseClass"))->as_class() } {
        if (env->false_obj()) NAT_UNREACHABLE();
    }

    ValuePtr to_s(Env *);

    virtual char *gc_repr() override {
        char *buf = new char[100];
        snprintf(buf, 100, "<FalseValue %p>", this);
        return buf;
    }
};

}
