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
struct typeinfo<int8_t> {
    constexpr const char *name() { return "int8_t"; }
};

template <>
struct typeinfo<std::size_t> {
    constexpr const char *name() { return "size_t"; }
};

#ifdef __APPLE__
template <>
struct typeinfo<uint64_t> {
    constexpr const char *name() { return "uint64_t"; }
};
#endif

}
