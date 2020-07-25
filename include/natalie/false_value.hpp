#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct FalseValue : Value {
    FalseValue(Env *env)
        : Value { Value::Type::False, env->Object()->const_get(env, "FalseClass", true)->as_class() } {
        if (env->false_obj()) NAT_UNREACHABLE();
    }

    Value *to_s(Env *);
};

}
