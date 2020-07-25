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
        if (env->true_obj()) return env->true_obj();
        return new TrueValue { env };
    }

    Value *to_s(Env *);

private:
    TrueValue(Env *env)
        : Value { Value::Type::True, env->Object()->const_get(env, "TrueClass", true)->as_class() } { }
};

}
