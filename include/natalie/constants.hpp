// This is used at build time as well in order to generate numbers.rb
#pragma once
#include "natalie/types.hpp"
#include <limits.h>

#define NAT_MAX_FIXNUM LLONG_MAX
#define NAT_MIN_FIXNUM LLONG_MIN + 1

namespace Natalie {

constexpr nat_int_t lowest_squarable_fixnum() {
    nat_int_t mask = (nat_int_t)1 << (sizeof(nat_int_t) * 8 - 2);
    nat_int_t max_sqrt = 0;
    while (mask > 0) {
        nat_int_t current_sum = max_sqrt + mask;
        mask >>= 1;
        if ((NAT_MAX_FIXNUM / current_sum) < current_sum) continue; // overflow
        max_sqrt = current_sum;
    }

    return max_sqrt;
}

constexpr nat_int_t NAT_MAX_FIXNUM_SQRT = lowest_squarable_fixnum();

}
