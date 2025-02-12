#pragma once

#include "natalie/bigint.hpp"
#include "natalie/constants.hpp"
#include "natalie/macros.hpp"
#include "natalie/types.hpp"
#include "tm/optional.hpp"
#include "tm/string.hpp"

#include <math.h>

namespace Natalie {

class Integer {
public:
    Integer(nat_int_t other) {
        if (other >= NAT_MIN_FIXNUM && other <= NAT_MAX_FIXNUM)
            m_value = left_shift_with_undefined_behavior(other, 1) | 0x1;
        else
            m_value = (uintptr_t) new BigInt(other);
    }

    Integer(long other) {
        if (other >= NAT_MIN_FIXNUM && other <= NAT_MAX_FIXNUM)
            m_value = left_shift_with_undefined_behavior(static_cast<nat_int_t>(other), 1) | 0x1;
        else
            m_value = (uintptr_t) new BigInt(other);
    }

    Integer(int other) {
        m_value = left_shift_with_undefined_behavior(static_cast<nat_int_t>(other), 1) | 0x1;
    }

    explicit Integer(BigInt *bigint)
        : m_value((uintptr_t)bigint) { }

    // This is hacky, but we need an Integer representation that
    // is all zeros so Value nullptr works. :-)
    static Integer null() {
        auto i = Integer(0);
        i.m_value = 0;
        return i;
    }

    Integer() { }
    Integer(double);
    Integer(const BigInt &);
    Integer(BigInt &&);
    Integer(const TM::String &);

    static bool will_multiplication_overflow(nat_int_t, nat_int_t);

    Integer &operator+=(const Integer &);
    Integer &operator-=(const Integer &);
    Integer &operator*=(const Integer &);
    Integer &operator/=(const Integer &);
    Integer &operator%=(const Integer &);
    Integer &operator>>=(const Integer &);

    // Unary operators
    // Integer operator+() const;
    Integer operator-() const;

    // Binary operators
    Integer operator+(const Integer &) const;
    Integer operator-(const Integer &) const;
    Integer operator*(const Integer &) const;
    Integer operator/(const Integer &) const;
    Integer div_c(const Integer &) const;
    Integer operator%(const Integer &) const;
    Integer modulo_c(const Integer &) const;

    double operator+(const double &) const;
    double operator-(const double &) const;
    double operator*(const double &) const;
    double operator/(const double &) const;

    Integer operator+(const nat_int_t &other) const { return *this + Integer(other); }
    Integer operator-(const nat_int_t &other) const { return *this - Integer(other); }
    Integer operator*(const nat_int_t &other) const { return *this * Integer(other); }
    Integer operator/(const nat_int_t &other) const { return *this / Integer(other); }
    Integer operator%(const nat_int_t &other) const { return *this % Integer(other); }
    Integer modulo_c(const nat_int_t &other) const { return this->modulo_c(Integer(other)); }

    Integer operator+(const int &other) const { return *this + Integer(other); }
    Integer operator-(const int &other) const { return *this - Integer(other); }
    Integer operator*(const int &other) const { return *this * Integer(other); }
    Integer operator/(const int &other) const { return *this / Integer(other); }
    Integer operator%(const int &other) const { return *this % Integer(other); }
    Integer modulo_c(const int &other) const { return this->modulo_c(Integer(other)); }

    // Increment and decrement operators
    Integer &operator++(); // pre-increment
    Integer &operator--(); // pre-decrement
    // Integer operator++(int); // post-increment
    // Integer operator--(int); // post-decrement

    // Relational operators
    bool operator<(const Integer &) const;
    bool operator>(const Integer &) const;
    bool operator<=(const Integer &) const;
    bool operator>=(const Integer &) const;
    bool operator==(const Integer &) const;
    bool operator!=(const Integer &) const;

    bool operator<(const nat_int_t &other) const { return *this < Integer(other); }
    bool operator>(const nat_int_t &other) const { return *this > Integer(other); }
    bool operator<=(const nat_int_t &other) const { return *this <= Integer(other); }
    bool operator>=(const nat_int_t &other) const { return *this >= Integer(other); }
    bool operator==(const nat_int_t &other) const { return *this == Integer(other); }
    bool operator!=(const nat_int_t &other) const { return *this != Integer(other); }

    bool operator<(const int &other) const { return *this < Integer(other); }
    bool operator>(const int &other) const { return *this > Integer(other); }
    bool operator<=(const int &other) const { return *this <= Integer(other); }
    bool operator>=(const int &other) const { return *this >= Integer(other); }
    bool operator==(const int &other) const { return *this == Integer(other); }
    bool operator!=(const int &other) const { return *this != Integer(other); }

    bool operator<(const double &) const;
    bool operator>(const double &) const;
    bool operator<=(const double &) const;
    bool operator>=(const double &) const;
    bool operator==(const double &) const;
    bool operator!=(const double &) const;

    bool operator<(const Value &) const = delete;
    bool operator>(const Value &) const = delete;
    bool operator<=(const Value &) const = delete;
    bool operator>=(const Value &) const = delete;
    bool operator==(const Value &) const = delete;
    bool operator!=(const Value &) const = delete;

    // Bitwise operators
    Integer operator&(const Integer &) const;
    Integer operator|(const Integer &) const;
    Integer operator^(const Integer &) const;
    Integer operator~() const;
    Integer operator<<(const Integer &) const;
    Integer operator>>(const Integer &) const;

    // Other
    explicit operator bool() const { return *this == 0; }
    bool is_zero() const { return *this == 0; }
    bool is_fixnum() const { return (m_value & 0x1) == 0x1; }
    bool is_bignum() const { return (m_value & 0x1) == 0x0; }
    bool is_negative() const;
    Integer bit_length() const;
    double to_double() const;
    TM::String to_string() const;

    BigInt to_bigint() const {
        if (is_bignum())
            return *(BigInt *)m_value;
        else
            return BigInt(to_nat_int_t());
    }

    nat_int_t to_nat_int_t() const {
        if (is_fixnum())
            return static_cast<nat_int_t>(m_value) >> 1;
        else if (*this >= LLONG_MIN && *this <= LLONG_MAX)
            return ((BigInt *)m_value)->to_long_long();
        else
            NAT_UNREACHABLE();
    }

    TM::Optional<nat_int_t> to_nat_int_t_or_none() const {
        if (is_fixnum())
            return static_cast<nat_int_t>(m_value) >> 1;
        else if (*this >= LLONG_MIN && *this <= LLONG_MAX)
            return ((BigInt *)m_value)->to_long_long();
        else
            return TM::Optional<nat_int_t> {};
    }

    BigInt *bigint_pointer() const {
        if (!is_bignum())
            return nullptr;
        return (BigInt *)m_value;
    }

private:
    friend Value;

    __attribute__((no_sanitize("undefined"))) static nat_int_t left_shift_with_undefined_behavior(nat_int_t x, nat_int_t y) {
        return x << y; // NOLINT
    }
    // The least significant bit is used to tag the pointer as either
    // an immediate value (63 bits) or a pointer to a BigInt.
    // If bit is 1, then shift the value to the right to get the actual
    // 63-bit number. If the bit is 0, then treat the value as a BigInt*.
    uintptr_t m_value { 0x1 };
};

Integer operator+(const long long &, const Integer &);
Integer operator-(const long long &, const Integer &);
Integer operator*(const long long &, const Integer &);
Integer operator/(const long long &, const Integer &);
Integer operator<(const long long &, const Integer &);
Integer operator>(const long long &, const Integer &);
Integer operator<=(const long long &, const Integer &);
Integer operator>=(const long long &, const Integer &);
Integer operator==(const long long &, const Integer &);
Integer operator!=(const long long &, const Integer &);
Integer pow(Integer, Integer);
Integer abs(const Integer &);
Integer gcd(const Integer &, const Integer &);
Integer sqrt(const Integer &);
}
