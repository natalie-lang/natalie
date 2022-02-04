#include "natalie/rounding_mode.hpp"

namespace Natalie {

RoundingMode rounding_mode_from_kwargs(Env *env, Value kwargs, RoundingMode default_rounding_mode) {
    if (!kwargs) return default_rounding_mode;
    if (!kwargs->is_hash()) return default_rounding_mode;

    auto value = kwargs->as_hash()->get(env, "half"_s);
    if (value->is_nil()) return RoundingMode::Up;
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
        env->raise("ArgumentError", "invalid rounding mode: {}", symbol->c_str());
    }
}

}
