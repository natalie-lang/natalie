#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class FalseValue : public Value {
public:
    FalseValue(Env *env)
        : Value { Value::Type::False, env->Object()->const_fetch(SymbolValue::intern("FalseClass"))->as_class() } {
        if (env->false_obj()) NAT_UNREACHABLE();
    }

    ValuePtr to_s(Env *);

    virtual void gc_print() override {
        fprintf(stderr, "<FalseValue %p>", this);
    }

    virtual bool is_collectible() override {
        return false;
    }
};

}
