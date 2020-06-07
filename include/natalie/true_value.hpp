#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct TrueValue : Value {
    static TrueValue *instance(Env *env) {
        if (NAT_TRUE) return NAT_TRUE;
        return new TrueValue { env };
    }

private:
    using Value::Value;

    TrueValue(Env *env)
        : Value { env, Value::Type::True, NAT_OBJECT->const_get(env, "TrueClass", true)->as_class() } { }
};

}
