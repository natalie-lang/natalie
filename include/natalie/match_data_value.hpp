#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct MatchDataValue : Value {
    using Value::Value;

    MatchDataValue(Env *env)
        : Value { env, Value::Type::MatchData, NAT_OBJECT->const_get(env, "MatchData", true)->as_class() } { }

    OnigRegion *matchdata_region;
    char *matchdata_str { nullptr };
};

}
