#pragma once

// https://github.com/983/bigint

#include "bigint.h"
#include "natalie/gc/cell.hpp"
#include "natalie/object.hpp"
#include "tm/string.hpp"

#include <algorithm>
#include <cfloat>
#include <limits>
#include <math.h>
#include <stdlib.h>
#include <type_traits>

namespace Natalie {

class BigInt : public Object {
public:
    BigInt()
        : Object { ObjectType::BigInt } {
        bigint_init(m_data);
    }

    BigInt(int b)
        : Object { ObjectType::BigInt } {
        bigint_init(m_data);
        bigint_from_int(m_data, b);
    }

    template <typename T>
    explicit BigInt(T b)
        : Object { ObjectType::BigInt } {
        bigint_init(m_data);
        typename std::make_unsigned<T>::type x;
        if constexpr (std::is_signed_v<T>) {
            x = std::abs(b);
        } else {
            x = b;
        }
        int n = std::max((long unsigned)1, sizeof(x) / sizeof(bigint_word));
        bigint_reserve(m_data, n);
        bigint_raw_zero(m_data->words, 0, n);
        memcpy(m_data->words, &x, sizeof(x));
        m_data->size = bigint_raw_truncate(m_data->words, n);
        if constexpr (std::is_signed_v<T>) {
            bigint_set_neg(m_data, b < 0);
        }
    }

    explicit BigInt(double b)
        : Object { ObjectType::BigInt } {
        int len = snprintf(nullptr, 0, "%.0f", b);
        char buf[len + 1];
        snprintf(buf, len + 1, "%.0f", b);
        bigint_init(m_data);
        bigint_from_str_base(m_data, buf, 10);
    }

    BigInt(const char *s, int base = 10)
        : Object { ObjectType::BigInt } {
        bigint_init(m_data);
        bigint_from_str_base(m_data, s, base);
    }

    BigInt(const TM::String &s, int base = 10)
        : Object { ObjectType::BigInt } {
        bigint_init(m_data);
        bigint_from_str_base(m_data, s.c_str(), base);
    }

    BigInt(const BigInt &b)
        : Object { ObjectType::BigInt } {
        bigint_init(m_data);
        bigint_cpy(m_data, b.m_data);
    }

    BigInt &operator=(const BigInt &b) {
        bigint_cpy(m_data, b.m_data);
        return *this;
    }

    ~BigInt() {
        bigint_free(m_data);
    }

    BigInt c_div(const BigInt &b) const {
        BigInt c;
        bigint_div(c.m_data, m_data, b.m_data);
        return c;
    }

    BigInt c_mod(const BigInt &b) const {
        BigInt c;
        bigint_mod(c.m_data, m_data, b.m_data);
        return c;
    }

    static void div_mod(
        BigInt &quotient, BigInt &remainder,
        const BigInt &biginterator,
        const BigInt &denominator) {
        bigint_div_mod(quotient.m_data, remainder.m_data,
            biginterator.m_data, denominator.m_data);
    }

    void write(
        char *dst,
        int *n_dst,
        int dst_base = 10,
        int zero_terminate = 1) const {
        bigint_write_base(dst, n_dst, m_data, dst_base, zero_terminate);
    }

    template <class STREAM>
    STREAM &write(STREAM &s, int dst_base = 10) const {
        int n = bigint_write_size(m_data, dst_base);
        char *buf = (char *)malloc(n);
        write(buf, &n, dst_base);
        s << buf;
        free(buf);
        return s;
    }

    BigInt &operator<<=(unsigned long long shift) {
        bigint_shift_left(m_data, m_data, shift);
        return *this;
    }

    BigInt &operator>>=(unsigned long long shift);

    BigInt &operator+=(const BigInt &b) {
        bigint_add(m_data, m_data, b.m_data);
        return *this;
    }

    BigInt &operator-=(const BigInt &b) {
        bigint_sub(m_data, m_data, b.m_data);
        return *this;
    }

    BigInt &operator*=(const BigInt &b) {
        bigint_mul(m_data, m_data, b.m_data);
        return *this;
    }

    BigInt &operator/=(const BigInt &b);

    BigInt &operator%=(const BigInt &b);

    BigInt &operator++() {
        bigint_add_word(m_data, m_data, 1);
        return *this;
    }

    BigInt &operator--() {
        bigint_sub_word(m_data, m_data, 1);
        return *this;
    }

    BigInt &operator&=(const BigInt &b) {
        bigint_bitwise_and(m_data, m_data, b.m_data);
        return *this;
    }

    BigInt &operator|=(const BigInt &b) {
        bigint_bitwise_or(m_data, m_data, b.m_data);
        return *this;
    }

    BigInt &operator^=(const BigInt &b) {
        bigint_bitwise_xor(m_data, m_data, b.m_data);
        return *this;
    }

    BigInt operator-() {
        BigInt b(*this);
        b.m_data->neg = !b.m_data->neg;
        return b;
    }

    bool operator==(const BigInt &b) const { return bigint_cmp(m_data, b.m_data) == 0; }
    bool operator!=(const BigInt &b) const { return bigint_cmp(m_data, b.m_data) != 0; }
    bool operator<=(const BigInt &b) const { return bigint_cmp(m_data, b.m_data) <= 0; }
    bool operator>=(const BigInt &b) const { return bigint_cmp(m_data, b.m_data) >= 0; }
    bool operator<(const BigInt &b) const { return bigint_cmp(m_data, b.m_data) < 0; }
    bool operator>(const BigInt &b) const { return bigint_cmp(m_data, b.m_data) > 0; }

    bool operator==(long long b) const { return bigint_cmp(m_data, BigInt(b).m_data) == 0; }
    bool operator!=(long long b) const { return bigint_cmp(m_data, BigInt(b).m_data) != 0; }
    bool operator<=(long long b) const { return bigint_cmp(m_data, BigInt(b).m_data) <= 0; }
    bool operator>=(long long b) const { return bigint_cmp(m_data, BigInt(b).m_data) >= 0; }
    bool operator<(long long b) const { return bigint_cmp(m_data, BigInt(b).m_data) < 0; }
    bool operator>(long long b) const { return bigint_cmp(m_data, BigInt(b).m_data) > 0; }

    bool operator==(const double &b) const {
        if (isinf(b)) return false;

        return *this == BigInt(floor(b));
    }

    bool operator<(const double &b) const {
        if (isinf(b)) return b > 0;

        return *this < BigInt(floor(b));
    }

    bool operator!=(const double &b) const { return !(*this == b); }
    bool operator>(const double &b) const { return !(*this < b) && !(*this == b); }
    bool operator<=(const double &b) const { return (*this < b) || (*this == b); }
    bool operator>=(const double &b) const { return !(*this < b); }

    BigInt operator+(const BigInt &b) const { return BigInt(*this) += b; }
    BigInt operator-(const BigInt &b) const { return BigInt(*this) -= b; }
    BigInt operator*(const BigInt &b) const { return BigInt(*this) *= b; }
    BigInt operator/(const BigInt &b) const { return BigInt(*this) /= b; }
    BigInt operator%(const BigInt &b) const { return BigInt(*this) %= b; }
    BigInt operator~() const { return BigInt(-1) - *this; }
    BigInt operator<<(long long shift) const { return BigInt(*this) <<= shift; }
    BigInt operator>>(long long shift) const { return BigInt(*this) >>= shift; }
    BigInt operator&(const BigInt &b) const { return BigInt(*this) &= b; }
    BigInt operator|(const BigInt &b) const { return BigInt(*this) |= b; }
    BigInt operator^(const BigInt &b) const { return BigInt(*this) ^= b; }

    BigInt &set_bit(int bit_index) {
        bigint_set_bit(m_data, bit_index);
        return *this;
    }

    BigInt &clr_bit(int bit_index) {
        bigint_clr_bit(m_data, bit_index);
        return *this;
    }

    bigint_word get_bit(int bit_index) const {
        return bigint_get_bit(m_data, bit_index);
    }

    int bitlength() const {
        return bigint_bitlength(m_data);
    }

    int count_trailing_zeros() const {
        return bigint_count_trailing_zeros(m_data);
    }

    BigInt sqrt() const {
        BigInt b;
        bigint_sqrt(b.m_data, m_data);
        return b;
    }

    BigInt pow(bigint_word exponent) {
        BigInt b;
        bigint_pow_word(b.m_data, m_data, exponent);
        return b;
    }

    long to_long() const {
        char buf[32];
        bigint_write(buf, 32, m_data);
        return strtol(buf, nullptr, 10);
    }

    long long to_long_long() const {
        char buf[32];
        bigint_write(buf, 32, m_data);
        return strtoll(buf, nullptr, 10);
    }

    inline bool is_negative() const { return m_data->neg == 1; }

    double to_double() const;
    TM::String to_string(int base = 10) const;
    TM::String to_binary() const;

    static BigInt gcd(const BigInt &a, const BigInt &b) {
        BigInt c;
        bigint_gcd(c.m_data, a.m_data, b.m_data);
        return c;
    }

    static BigInt rand_bits(int n_bits, bigint_rand_func rand_func) {
        BigInt b;
        bigint_rand_bits(b.m_data, n_bits, rand_func);
        return b;
    }

    static BigInt rand_inclusive(const BigInt &n, bigint_rand_func rand_func) {
        BigInt b;
        bigint_rand_inclusive(b.m_data, n.m_data, rand_func);
        return b;
    }

    static BigInt rand_exclusive(const BigInt &n, bigint_rand_func rand_func) {
        BigInt b;
        bigint_rand_exclusive(b.m_data, n.m_data, rand_func);
        return b;
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<BigInt %p value=%s>", this, to_string().c_str());
    }

private:
    bigint m_data[1];
};

}
