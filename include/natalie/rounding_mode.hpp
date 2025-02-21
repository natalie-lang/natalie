#pragma once

#include "natalie.hpp"

namespace Natalie {

enum class RoundingMode {
    Up,
    Down,
    Even,
};

RoundingMode rounding_mode_from_value(Env *env, Optional<Value> value, RoundingMode default_rounding_mode = RoundingMode::Up);

}
