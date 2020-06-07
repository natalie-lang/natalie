#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ProcValue : Value {
    using Value::Value;

    ProcValue(Env *env)
        : Value { env, Value::Type::Proc, NAT_OBJECT->const_get(env, "Proc", true)->as_class() } { }

    Block *block { nullptr };
    bool lambda;
};

}
