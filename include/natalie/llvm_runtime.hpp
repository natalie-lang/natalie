#pragma once

#include "natalie.hpp"
#include <cstring>
#include <type_traits>

// The LLVM boundary passes tagged Values as machine words, never C++ classes.
static_assert(sizeof(Natalie::Value) == sizeof(uint64_t));
static_assert(std::is_trivially_copyable_v<Natalie::Value>);
static_assert(Natalie::FALSE_VALUE == 0 && Natalie::NIL_VALUE == 4 && Natalie::TRUE_VALUE == 20);

inline uint64_t nat_llvm_pack(Natalie::Value value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

inline Natalie::Value nat_llvm_unpack(uint64_t bits) {
    Natalie::Value value;
    memcpy(static_cast<void *>(&value), &bits, sizeof(value));
    return value;
}

extern "C" uint64_t nat_llvm_eval(Natalie::Env *, uint64_t);
