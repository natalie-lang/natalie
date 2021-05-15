#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class TrueValue : public Value {
public:
    TrueValue(Env *env)
        : Value { Value::Type::True, env->Object()->const_fetch(env, SymbolValue::intern(env, "TrueClass"))->as_class() } {
        if (env->true_obj()) NAT_UNREACHABLE();
    }

    ValuePtr to_s(Env *);

    virtual void gc_print() override {
        fprintf(stderr, "<TrueValue %p>", this);
    }
};

}
