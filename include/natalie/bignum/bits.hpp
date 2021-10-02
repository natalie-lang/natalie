/* Copyright (C) 2017, Michael Clark <michaeljclark@mac.com>
 * Copyright (C) 2018, SiFive, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----
 *
 * Large portions of this file were copied from Michael Clark's bignum library
 * made available at: https://github.com/michaeljclark/bignum
 */

#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

/*! clz */
template <typename T>
inline int clz(T val) {
    const int bits = sizeof(T) << 3;
    unsigned count = 0, found = 0;
    for (int i = bits - 1; i >= 0; --i) {
        count += !(found |= val & T(1) << i ? 1 : 0);
    }
    return count;
}

/*! ctz */
template <typename T>
inline int ctz(T val) {
    const int bits = sizeof(T) << 3;
    unsigned count = 0, found = 0;
    for (int i = 0; i < bits; ++i) {
        count += !(found |= val & T(1) << i ? 1 : 0);
    }
    return count;
}

/* ctz specializations */
#if defined(__GNUC__)
template <>
inline int clz(unsigned val) { return __builtin_clz(val); }
template <>
inline int clz(unsigned long val) { return __builtin_clzll(val); }
template <>
inline int clz(unsigned long long val) { return __builtin_clzll(val); }
template <>
inline int ctz(unsigned val) { return __builtin_ctz(val); }
template <>
inline int ctz(unsigned long val) { return __builtin_ctzll(val); }
template <>
inline int ctz(unsigned long long val) { return __builtin_ctzll(val); }
#endif
#if defined(_MSC_VER)
#if defined(_M_X64)
template <>
inline int clz(unsigned val) {
    return (int)_lzcnt_u32(val);
}
template <>
inline int clz(unsigned long long val) {
    return (int)_lzcnt_u64(val);
}
template <>
inline int ctz(unsigned val) {
    return (int)_tzcnt_u32(val);
}
template <>
inline int ctz(unsigned long long val) {
    return (int)_tzcnt_u64(val);
}
#else
template <>
inline int clz(unsigned val) {
    unsigned long count;
    return _BitScanReverse(&count, val) ^ 31;
}
template <>
inline int ctz(unsigned val) {
    unsigned long count;
    return _BitScanForward(&count, val);
}
#endif
#endif
