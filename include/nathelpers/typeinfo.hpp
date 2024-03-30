#pragma once

#include <cstdint>

namespace Natalie {

template <class T>
struct typeinfo {
    constexpr const char *name() = delete;
};

template <>
struct typeinfo<int> {
    constexpr const char *name() { return "int"; }
};

template <>
struct typeinfo<std::size_t> {
    constexpr const char *name() { return "size_t"; }
};

}
