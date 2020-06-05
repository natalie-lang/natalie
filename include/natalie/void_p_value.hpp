#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/encoding_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct VoidPValue : Value {
    using Value::Value;

    VoidPValue(Env *env)
        : Value { env, Value::Type::VoidP, nullptr } { }

    void *void_ptr { nullptr };
};

}
