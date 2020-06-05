#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct NilValue : Value {
    static NilValue *instance(Env *env) {
        if (NAT_NIL) return NAT_NIL;
        return new NilValue { env };
    }

private:
    using Value::Value;

    NilValue(Env *env)
        : Value { env, Value::Type::Nil, const_get(env, NAT_OBJECT, "NilClass", true)->as_class() } { }
};

}
