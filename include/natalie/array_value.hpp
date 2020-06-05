#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"
#include "natalie/vector_temporary_delete_this_soon.hpp"

namespace Natalie {

struct ArrayValue : Value {
    using Value::Value;

    ArrayValue(Env *env);

    Vector ary;
};

}
