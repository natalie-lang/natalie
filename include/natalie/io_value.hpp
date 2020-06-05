#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct IoValue : Value {
    using Value::Value;

    IoValue(Env *env)
        : Value { env, Value::Type::Io, const_get(env, NAT_OBJECT, "IO", true)->as_class() } { }

    IoValue(Env *env, ClassValue *klass)
        : Value { env, Value::Type::Io, klass } { }

    int fileno { 0 };
};

}
