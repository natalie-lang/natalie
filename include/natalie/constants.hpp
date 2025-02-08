// This is used at build time as well in order to generate numbers.rb
#pragma once
#include "natalie/types.hpp"
#include <limits.h>

namespace Natalie {

constexpr nat_int_t highest_fixnum() {
    auto bits = sizeof(nat_int_t) * 8 - 2;
    return (1LL << bits) - 1;
}

constexpr nat_int_t lowest_fixnum() {
    auto bits = sizeof(nat_int_t) * 8 - 2;
    return -(1LL << bits);
}

constexpr nat_int_t NAT_MAX_FIXNUM = highest_fixnum();
constexpr nat_int_t NAT_MIN_FIXNUM = lowest_fixnum();

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
