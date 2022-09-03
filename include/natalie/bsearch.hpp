#pragma once

#include "natalie.hpp"

namespace Natalie {
enum class BSearchCheckResult {
    SMALLER,
    BIGGER,
    EQUAL
};

BSearchCheckResult binary_search_check(Env *, Value);
Optional<nat_int_t> binary_search(Env *, nat_int_t, nat_int_t, std::function<Value(nat_int_t)>);
Value binary_search_integer(Env *, nat_int_t, nat_int_t, Block *, bool = false);
Value binary_search_float(Env *, double, double, Block *, bool = false);

}
