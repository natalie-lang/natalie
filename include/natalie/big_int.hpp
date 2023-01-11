/*
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2018 Syed Faheel Ahmad
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ----
 *
 * Large portions of this file were copied from Syed Faheel Ahmad's BigInt library
 * made available at: https://github.com/faheel/BigInt
 *
 */

/*
    BigInt
    ------
    Arbitrary-sized integer class for C++.

    Version: 0.5.0-dev
    Released on: 05 October 2020 23:15 IST
    Author: Syed Faheel Ahmad (faheel@live.in)
    Project on GitHub: https://github.com/faheel/BigInt
    License: MIT
*/

/*
    ===========================================================================
    BigInt
    ===========================================================================
    Definition for the BigInt class.
*/

#pragma once

#include "tm/string.hpp"

class BigInt {
public:
    // Constructors:
    BigInt();
    BigInt(const BigInt &);
    BigInt(BigInt &&);
    BigInt(const long long &);
    BigInt(const unsigned long int &);
    BigInt(const unsigned long long &);
    BigInt(const long &);
    BigInt(const unsigned int &);
    BigInt(const int &);
    BigInt(const double &);
    BigInt(const TM::String &, const int base = 10);

    // Assignment operators:
    BigInt &operator=(const BigInt &);
    BigInt &operator=(BigInt &&);
    BigInt &operator=(const long long &);
    BigInt &operator=(const TM::String &);

    // Unary arithmetic operators:
    BigInt operator+() const; // unary +
    BigInt operator-() const; // unary -

    // Binary arithmetic operators:
    BigInt operator+(const BigInt &) const;
    BigInt operator-(const BigInt &) const;
    BigInt operator*(const BigInt &) const;
    BigInt operator/(const BigInt &) const;
    BigInt c_div(const BigInt &) const;
    BigInt operator%(const BigInt &) const;
    BigInt c_mod(const BigInt &) const;
    BigInt operator+(const long long &) const;
    BigInt operator-(const long long &) const;
    BigInt operator*(const long long &) const;
    BigInt operator/(const long long &) const;
    BigInt operator%(const long long &) const;
    BigInt operator+(const TM::String &) const;
    BigInt operator-(const TM::String &) const;
    BigInt operator*(const TM::String &) const;
    BigInt operator/(const TM::String &) const;
    BigInt operator%(const TM::String &) const;

    // Arithmetic-assignment operators:
    BigInt &operator+=(const BigInt &);
    BigInt &operator-=(const BigInt &);
    BigInt &operator*=(const BigInt &);
    BigInt &operator/=(const BigInt &);
    BigInt &operator%=(const BigInt &);
    BigInt &operator+=(const long long &);
    BigInt &operator-=(const long long &);
    BigInt &operator*=(const long long &);
    BigInt &operator/=(const long long &);
    BigInt &operator%=(const long long &);
    BigInt &operator+=(const TM::String &);
    BigInt &operator-=(const TM::String &);
    BigInt &operator*=(const TM::String &);
    BigInt &operator/=(const TM::String &);
    BigInt &operator%=(const TM::String &);

    // Increment and decrement operators:
    BigInt &operator++(); // pre-increment
    BigInt &operator--(); // pre-decrement
    BigInt operator++(int); // post-increment
    BigInt operator--(int); // post-decrement

    // Relational operators:
    bool operator<(const BigInt &) const;
    bool operator>(const BigInt &) const;
    bool operator<=(const BigInt &) const;
    bool operator>=(const BigInt &) const;
    bool operator==(const BigInt &) const;
    bool operator!=(const BigInt &) const;
    bool operator<(const long long &) const;
    bool operator>(const long long &) const;
    bool operator<=(const long long &) const;
    bool operator>=(const long long &) const;
    bool operator==(const long long &) const;
    bool operator!=(const long long &) const;
    bool operator<(const int &) const;
    bool operator>(const int &) const;
    bool operator<=(const int &) const;
    bool operator>=(const int &) const;
    bool operator==(const int &) const;
    bool operator!=(const int &) const;
    bool operator<(const TM::String &) const;
    bool operator>(const TM::String &) const;
    bool operator<=(const TM::String &) const;
    bool operator>=(const TM::String &) const;
    bool operator==(const TM::String &) const;
    bool operator!=(const TM::String &) const;
    bool operator<(const double &) const;
    bool operator>(const double &) const;
    bool operator<=(const double &) const;
    bool operator>=(const double &) const;
    bool operator==(const double &) const;
    bool operator!=(const double &) const;

    // Bitwise operators:
    BigInt operator&(const BigInt &num) const;
    BigInt operator|(const BigInt &num) const;
    BigInt operator^(const BigInt &num) const;
    BigInt operator~() const;

    BigInt operator<<(const size_t &num) const;
    BigInt operator>>(const size_t &num) const;

    bool is_negative() const { return m_sign == '-'; }

    // Conversion functions:
    TM::String to_string() const;
    TM::String to_binary() const;
    long to_long() const;
    long long to_long_long() const;
    double to_double() const;

    // Random number generating functions:
    friend BigInt big_random(size_t);

private:
    TM::String m_value;
    char m_sign;
};

BigInt pow(const BigInt &base, long long exp);
BigInt gcd(const BigInt &num1, const BigInt &num2);
BigInt sqrt(const BigInt &num);
BigInt operator+(const long long &lhs, const BigInt &rhs);
BigInt operator-(const long long &lhs, const BigInt &rhs);
BigInt operator*(const long long &lhs, const BigInt &rhs);
BigInt operator/(const long long &lhs, const BigInt &rhs);
BigInt operator%(const long long &lhs, const BigInt &rhs);
bool operator<(const long long &lhs, const BigInt &rhs);
bool operator==(const long long &lhs, const BigInt &rhs);

BigInt abs(const BigInt &num);
BigInt big_pow10(size_t exp);
