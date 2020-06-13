#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct IoValue : Value {
    IoValue(Env *env)
        : Value { Value::Type::Io, NAT_OBJECT->const_get(env, "IO", true)->as_class() } { }

    IoValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Io, klass } { }

    int fileno { 0 };
};

}
