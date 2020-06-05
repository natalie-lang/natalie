#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct FalseValue : Value {
    static FalseValue *instance(Env *env) {
        if (NAT_FALSE) return NAT_FALSE;
        return new FalseValue { env };
    }

private:
    using Value::Value;

    FalseValue(Env *env)
        : Value { env, Value::Type::False, const_get(env, NAT_OBJECT, "FalseClass", true)->as_class() } { }
};

}
