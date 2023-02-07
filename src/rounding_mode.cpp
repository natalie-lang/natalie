#include "natalie/rounding_mode.hpp"

namespace Natalie {

RoundingMode rounding_mode_from_value(Env *env, Value value, RoundingMode default_rounding_mode) {
    if (!value) return default_rounding_mode;
    if (value->is_nil()) return default_rounding_mode;
    if (!value->is_symbol()) {
        env->raise("ArgumentError", "invalid rounding mode: {}", value->inspect_str(env));
    }

    auto symbol = value->as_symbol();

    if (symbol == "up"_s)
        return RoundingMode::Up;
    else if (symbol == "down"_s)
        return RoundingMode::Down;
    else if (symbol == "even"_s)
        return RoundingMode::Even;
    else {
        env->raise("ArgumentError", "invalid rounding mode: {}", symbol->string());
    }
}

}
