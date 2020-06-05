#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct RangeValue : Value {
    using Value::Value;

    RangeValue(Env *env)
        : Value { env, Value::Type::Range, const_get(env, NAT_OBJECT, "Range", true)->as_class() } { }

    Value *range_begin { nullptr };
    Value *range_end { nullptr };
    bool range_exclude_end { false };
};

}
