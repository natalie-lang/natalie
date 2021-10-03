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

#include <iostream>

class BigInt {
    std::string value;
    char sign;

public:
    // Constructors:
    BigInt();
    BigInt(const BigInt &);
    BigInt(const long long &);
    BigInt(const std::string &);

    // Assignment operators:
    BigInt &operator=(const BigInt &);
    BigInt &operator=(const long long &);
    BigInt &operator=(const std::string &);

    // Unary arithmetic operators:
    BigInt operator+() const; // unary +
    BigInt operator-() const; // unary -

    // Binary arithmetic operators:
    BigInt operator+(const BigInt &) const;
    BigInt operator-(const BigInt &) const;
    BigInt operator*(const BigInt &) const;
    BigInt operator/(const BigInt &) const;
    BigInt operator%(const BigInt &) const;
    BigInt operator+(const long long &) const;
    BigInt operator-(const long long &) const;
    BigInt operator*(const long long &) const;
    BigInt operator/(const long long &) const;
    BigInt operator%(const long long &) const;
    BigInt operator+(const std::string &) const;
    BigInt operator-(const std::string &) const;
    BigInt operator*(const std::string &) const;
    BigInt operator/(const std::string &) const;
    BigInt operator%(const std::string &) const;

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
    BigInt &operator+=(const std::string &);
    BigInt &operator-=(const std::string &);
    BigInt &operator*=(const std::string &);
    BigInt &operator/=(const std::string &);
    BigInt &operator%=(const std::string &);

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
    bool operator<(const std::string &) const;
    bool operator>(const std::string &) const;
    bool operator<=(const std::string &) const;
    bool operator>=(const std::string &) const;
    bool operator==(const std::string &) const;
    bool operator!=(const std::string &) const;

    // I/O stream operators:
    friend std::istream &operator>>(std::istream &, BigInt &);
    friend std::ostream &operator<<(std::ostream &, const BigInt &);

    // Conversion functions:
    std::string to_string() const;
    int to_int() const;
    long to_long() const;
    long long to_long_long() const;

    // Random number generating functions:
    friend BigInt big_random(size_t);
};
