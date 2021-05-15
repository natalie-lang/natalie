#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class NilValue : public Value {
public:
    NilValue(Env *env)
        : Value { Value::Type::Nil, env->Object()->const_fetch(env, SymbolValue::intern(env, "NilClass"))->as_class() } {
        if (env->nil_obj()) NAT_UNREACHABLE();
    }

    ValuePtr eqtilde(Env *, ValuePtr);
    ValuePtr to_s(Env *);
    ValuePtr to_a(Env *);
    ValuePtr to_i(Env *);
    ValuePtr inspect(Env *);

    virtual void gc_print() override {
        fprintf(stderr, "<NilValue %p>", this);
    }
};

}
