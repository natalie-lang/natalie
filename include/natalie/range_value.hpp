#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct RangeValue : Value {
    RangeValue(Env *env)
        : Value { Value::Type::Range, NAT_OBJECT->const_get(env, "Range", true)->as_class() } { }

    Value *range_begin { nullptr };
    Value *range_end { nullptr };
    bool range_exclude_end { false };
};

}
