#pragma once

#include "natalie.hpp"

namespace Natalie {

enum class RoundingMode {
    Up,
    Down,
    Even,
};

RoundingMode rounding_mode_from_kwargs(Env *env, Value kwargs, RoundingMode default_rounding_mode = RoundingMode::Up);

}
