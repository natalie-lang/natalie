#pragma once

// https://github.com/983/bigint

#include "bigint.h"
#include "tm/string.hpp"

#include <algorithm>
#include <cfloat>
#include <limits>
#include <math.h>
#include <stdlib.h>
#include <type_traits>

namespace Natalie {

struct BigInt {
    bigint data[1];

    BigInt() {
        bigint_init(data);
    }

    BigInt(int b) {
        bigint_init(data);
        bigint_from_int(data, b);
    }

    template <typename T>
    explicit BigInt(T b) {
        bigint_init(data);
        typename std::make_unsigned<T>::type x;
        if constexpr (std::is_signed_v<T>) {
            x = std::abs(b);
        } else {
            x = b;
        }
        int n = std::max((long unsigned)1, sizeof(x) / sizeof(bigint_word));
        bigint_reserve(data, n);
        bigint_raw_zero(data->words, 0, n);
        memcpy(data->words, &x, sizeof(x));
        data->size = bigint_raw_truncate(data->words, n);
        if constexpr (std::is_signed_v<T>) {
            bigint_set_neg(data, b < 0);
        }
    }

    explicit BigInt(double b) {
        int len = snprintf(nullptr, 0, "%.0f", b);
        char buf[len + 1];
        snprintf(buf, len + 1, "%.0f", b);
        bigint_init(data);
        bigint_from_str_base(data, buf, 10);
    }

    BigInt(const char *s, int base = 10) {
        bigint_init(data);
        bigint_from_str_base(data, s, base);
    }

    BigInt(const TM::String &s, int base = 10) {
        bigint_init(data);
        bigint_from_str_base(data, s.c_str(), base);
    }

    BigInt(const BigInt &b) {
        bigint_init(data);
        bigint_cpy(data, b.data);
    }

    BigInt &operator=(const BigInt &b) {
        bigint_cpy(data, b.data);
        return *this;
    }

    ~BigInt() {
        bigint_free(data);
    }

    BigInt c_div(const BigInt &b) const {
        BigInt c;
        bigint_div(c.data, data, b.data);
        return c;
    }

    BigInt c_mod(const BigInt &b) const {
        BigInt c;
        bigint_mod(c.data, data, b.data);
        return c;
    }

    static void div_mod(
        BigInt &quotient, BigInt &remainder,
        const BigInt &biginterator,
        const BigInt &denominator) {
        bigint_div_mod(quotient.data, remainder.data,
            biginterator.data, denominator.data);
    }

    void write(
        char *dst,
        int *n_dst,
        int dst_base = 10,
        int zero_terminate = 1) const {
        bigint_write_base(dst, n_dst, data, dst_base, zero_terminate);
    }

    template <class STREAM>
    STREAM &write(STREAM &s, int dst_base = 10) const {
        int n = bigint_write_size(data, dst_base);
        char *buf = (char *)malloc(n);
        write(buf, &n, dst_base);
        s << buf;
        free(buf);
        return s;
    }

    BigInt &operator<<=(unsigned long long shift) {
        bigint_shift_left(data, data, shift);
        return *this;
    }

    BigInt &operator>>=(unsigned long long shift);

    BigInt &operator+=(const BigInt &b) {
        bigint_add(data, data, b.data);
        return *this;
    }

    BigInt &operator-=(const BigInt &b) {
        bigint_sub(data, data, b.data);
        return *this;
    }

    BigInt &operator*=(const BigInt &b) {
        bigint_mul(data, data, b.data);
        return *this;
    }

    BigInt &operator/=(const BigInt &b);

    BigInt &operator%=(const BigInt &b);

    BigInt &operator++() {
        bigint_add_word(data, data, 1);
        return *this;
    }

    BigInt &operator--() {
        bigint_sub_word(data, data, 1);
        return *this;
    }

    BigInt &operator&=(const BigInt &b) {
        bigint_bitwise_and(data, data, b.data);
        return *this;
    }

    BigInt &operator|=(const BigInt &b) {
        bigint_bitwise_or(data, data, b.data);
        return *this;
    }

    BigInt &operator^=(const BigInt &b) {
        bigint_bitwise_xor(data, data, b.data);
        return *this;
    }

    BigInt &set_bit(int bit_index) {
        bigint_set_bit(data, bit_index);
        return *this;
    }

    BigInt &clr_bit(int bit_index) {
        bigint_clr_bit(data, bit_index);
        return *this;
    }

    bigint_word get_bit(int bit_index) const {
        return bigint_get_bit(data, bit_index);
    }

    int bitlength() const {
        return bigint_bitlength(data);
    }

    int count_trailing_zeros() const {
        return bigint_count_trailing_zeros(data);
    }

    BigInt sqrt() const {
        BigInt b;
        bigint_sqrt(b.data, data);
        return b;
    }

    BigInt pow(bigint_word exponent) {
        BigInt b;
        bigint_pow_word(b.data, data, exponent);
        return b;
    }

    long to_long() const {
        char buf[32];
        bigint_write(buf, 32, data);
        return strtol(buf, nullptr, 10);
    }

    long to_long_long() const {
        char buf[32];
        bigint_write(buf, 32, data);
        return strtoll(buf, nullptr, 10);
    }

    inline bool is_negative() const { return data->neg == 1; }

    double to_double() const;
    TM::String to_string(int base = 10) const;
    TM::String to_binary() const;

    static BigInt gcd(const BigInt &a, const BigInt &b) {
        BigInt c;
        bigint_gcd(c.data, a.data, b.data);
        return c;
    }

    static BigInt rand_bits(int n_bits, bigint_rand_func rand_func) {
        BigInt b;
        bigint_rand_bits(b.data, n_bits, rand_func);
        return b;
    }

    static BigInt rand_inclusive(const BigInt &n, bigint_rand_func rand_func) {
        BigInt b;
        bigint_rand_inclusive(b.data, n.data, rand_func);
        return b;
    }

    static BigInt rand_exclusive(const BigInt &n, bigint_rand_func rand_func) {
        BigInt b;
        bigint_rand_exclusive(b.data, n.data, rand_func);
        return b;
    }
};

inline BigInt operator-(const BigInt &a) {
    BigInt b(a);
    b.data->neg = !b.data->neg;
    return b;
}

inline BigInt operator+(const BigInt &a, const BigInt &b) {
    return BigInt(a) += b;
}

inline BigInt operator-(const BigInt &a, const BigInt &b) {
    return BigInt(a) -= b;
}

inline BigInt operator*(const BigInt &a, const BigInt &b) {
    return BigInt(a) *= b;
}

inline BigInt operator/(const BigInt &a, const BigInt &b) {
    return BigInt(a) /= b;
}

inline BigInt operator%(const BigInt &a, const BigInt &b) {
    return BigInt(a) %= b;
}

inline BigInt operator~(const BigInt &a) {
    return BigInt(-1) - a;
}

inline BigInt operator<<(const BigInt &a, long long shift) {
    return BigInt(a) <<= shift;
}

inline BigInt operator>>(const BigInt &a, long long shift) {
    return BigInt(a) >>= shift;
}

inline BigInt operator&(const BigInt &a, const BigInt &b) {
    return BigInt(a) &= b;
}

inline BigInt operator|(const BigInt &a, const BigInt &b) {
    return BigInt(a) |= b;
}

inline BigInt operator^(const BigInt &a, const BigInt &b) {
    return BigInt(a) ^= b;
}

inline bool operator==(const BigInt &a, const BigInt &b) { return bigint_cmp(a.data, b.data) == 0; }
inline bool operator!=(const BigInt &a, const BigInt &b) { return bigint_cmp(a.data, b.data) != 0; }
inline bool operator<=(const BigInt &a, const BigInt &b) { return bigint_cmp(a.data, b.data) <= 0; }
inline bool operator>=(const BigInt &a, const BigInt &b) { return bigint_cmp(a.data, b.data) >= 0; }
inline bool operator<(const BigInt &a, const BigInt &b) { return bigint_cmp(a.data, b.data) < 0; }
inline bool operator>(const BigInt &a, const BigInt &b) { return bigint_cmp(a.data, b.data) > 0; }

inline bool operator==(const BigInt &a, long long b) { return bigint_cmp(a.data, BigInt(b).data) == 0; }
inline bool operator!=(const BigInt &a, long long b) { return bigint_cmp(a.data, BigInt(b).data) != 0; }
inline bool operator<=(const BigInt &a, long long b) { return bigint_cmp(a.data, BigInt(b).data) <= 0; }
inline bool operator>=(const BigInt &a, long long b) { return bigint_cmp(a.data, BigInt(b).data) >= 0; }
inline bool operator<(const BigInt &a, long long b) { return bigint_cmp(a.data, BigInt(b).data) < 0; }
inline bool operator>(const BigInt &a, long long b) { return bigint_cmp(a.data, BigInt(b).data) > 0; }

inline bool operator==(const BigInt &a, const double &b) {
    if (isinf(b)) return false;

    return a == BigInt(floor(b));
}

inline bool operator<(const BigInt &a, const double &b) {
    if (isinf(b)) return b > 0;

    return a < BigInt(floor(b));
}

inline bool operator!=(const BigInt &a, const double &b) { return !(a == b); }
inline bool operator>(const BigInt &a, const double &b) { return !(a < b) && !(a == b); }
inline bool operator<=(const BigInt &a, const double &b) { return (a < b) || (a == b); }
inline bool operator>=(const BigInt &a, const double &b) { return !(a < b); }

}
