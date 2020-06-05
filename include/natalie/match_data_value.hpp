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
        : Value { env, Value::Type::MatchData, const_get(env, NAT_OBJECT, "MatchData", true)->as_class() } { }

    OnigRegion *matchdata_region;
    char *matchdata_str { nullptr };
};

}
