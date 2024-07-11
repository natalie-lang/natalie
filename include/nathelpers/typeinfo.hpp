#pragma once

#include <cstdint>
#include <cstdio>

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
struct typeinfo<size_t> {
    constexpr const char *name() { return "size_t"; }
};

template <>
struct typeinfo<ssize_t> {
    constexpr const char *name() { return "ssize_t"; }
};

template <>
struct typeinfo<long long int> {
    constexpr const char *name() { return "long long int"; }
};

#ifdef __APPLE__
template <>
struct typeinfo<uint64_t> {
    constexpr const char *name() { return "uint64_t"; }
};
#endif

}
