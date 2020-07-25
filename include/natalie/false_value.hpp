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
        if (env->false_obj()) return env->false_obj();
        return new FalseValue { env };
    }

    Value *to_s(Env *);

private:
    FalseValue(Env *env)
        : Value { Value::Type::False, env->Object()->const_get(env, "FalseClass", true)->as_class() } { }
};

}
