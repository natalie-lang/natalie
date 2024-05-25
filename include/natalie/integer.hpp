#pragma once

#include "natalie/bigint.hpp"
#include "natalie/macros.hpp"
#include "natalie/types.hpp"
#include "tm/string.hpp"

#include <math.h>

namespace Natalie {

class Integer {
public:
    Integer() { }
    Integer(nat_int_t);
    Integer(int);
    Integer(double);
    Integer(const BigInt &);
    Integer(BigInt &&);
    Integer(const TM::String &);
    Integer(const Integer &);
    Integer(Integer &&);

    static bool will_multiplication_overflow(nat_int_t, nat_int_t);

    // Assignment operators
    Integer &operator=(const Integer &);
    Integer &operator=(Integer &&);
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

    // Bitwise operators
    Integer operator&(const Integer &) const;
    Integer operator|(const Integer &) const;
    Integer operator^(const Integer &) const;
    Integer operator~() const;
    Integer operator<<(const Integer &) const;
    Integer operator>>(const Integer &) const;

    // Other
    explicit operator bool() const { return *this == 0; }
    bool is_fixnum() const { return m_is_fixnum; }
    bool is_bignum() const { return !m_is_fixnum; }
    bool is_negative() const;
    BigInt to_bigint() const {
        if (is_bignum())
            return *m_bignum;
        else
            return BigInt(m_fixnum);
    }
    nat_int_t to_nat_int_t() const {
        if (is_fixnum())
            return m_fixnum;
        else
            NAT_UNREACHABLE();
    }
    double to_double() const;
    TM::String to_string() const;

    Integer bit_length() const;
    // TM::String to_binary() const;

    ~Integer() {
        if (is_bignum())
            delete m_bignum;
    }

private:
    union {
        nat_int_t m_fixnum { 0 };
        BigInt *m_bignum;
    };

    bool m_is_fixnum { true };
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
