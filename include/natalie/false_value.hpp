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
        : Value { Value::Type::False, env->Object()->const_fetch("FalseClass")->as_class() } {
        if (env->false_obj()) NAT_UNREACHABLE();
    }

    ValuePtr to_s(Env *);
};

}
