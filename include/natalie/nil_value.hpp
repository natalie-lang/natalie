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
    NilValue(Env *env)
        : Value { Value::Type::Nil, NAT_OBJECT->const_get(env, "NilClass", true)->as_class() } { }
};

}
