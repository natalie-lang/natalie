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

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "bits.hpp"
#include "hostint.hpp"

/*------------------.
| signedness.       |
`------------------*/

struct is_signed { };
struct is_unsigned { };

struct signedness {
    bool is_signed;

    signedness() = delete;
    signedness(struct is_signed)
        : is_signed(true) { }
    signedness(struct is_unsigned)
        : is_signed(false) { }
    signedness(const signedness &o) = default;
    signedness &operator=(const signedness &) = default;
};

/*------------------.
| bitwidth.         |
`------------------*/

struct bitwidth {
    unsigned width;

    bitwidth() = delete;
    bitwidth(unsigned bits)
        : width(bits) { }
    bitwidth &operator=(const bitwidth &) = default;
    operator unsigned() const { return width; }
};

/*------------------.
| bignum.           |
`------------------*/

struct bignum {
    /*------------------.
	| type definitions. |
	`------------------*/

    /*! limb bit width and bit shift */
    enum {
        limb_bits = 32,
        limb_shift = 5,
    };

    /*! limb type */
    typedef hostint<limb_bits, false>::type ulimb_t;
    typedef hostint<limb_bits * 2, false>::type udlimb_t;
    typedef hostint<limb_bits * 2, true>::type sdlimb_t;

    /*------------------.
	| member variables. |
	`------------------*/

    /* limbs is a vector of words with the little end at offset 0 */
    std::vector<ulimb_t> limbs;

    /*! flags indicating unsigned or signed two's complement */
    signedness s;

    /*! contains the width of the bit vector in bits (variable width = 0) */
    bitwidth bits;

    /*--------------.
	| constructors. |
	`--------------*/

    /*! empty constructor */
    bignum(const signedness s = is_unsigned(), const bitwidth bits = 0);

    /*! integral constructor */
    bignum(const ulimb_t n, const signedness s = is_unsigned(), const bitwidth bits = 0);

    /*! array constructor */
    bignum(const std::initializer_list<ulimb_t> l, const signedness s = is_unsigned(), const bitwidth bits = 0);

    /*! string constructor */
    bignum(std::string str, const signedness s = is_unsigned(), const bitwidth bits = 0);

    /*! string constructor with radix */
    bignum(std::string str, const size_t radix, const signedness s = is_unsigned(), const bitwidth bits = 0);

    /*! copy constructor */
    bignum(const bignum &operand);

    /*! move constructor */
    bignum(const bignum &&operand) noexcept;

    /*----------------------.
	| assignment operators. |
	`----------------------*/

    /*! integral copy assignment operator */
    bignum &operator=(const ulimb_t l);

    /*! bignum copy assignment operator */
    bignum &operator=(const bignum &operand);

    /*! bignum move assignment operator */
    bignum &operator=(bignum &&operand);

    /*------------------.
	| internal methods. |
	`------------------*/

    /*! expand limbs to match operand */
    void _expand(const bignum &operand);

    /*! contract zero big end limbs */
    void _contract();

    /*! resize number of limbs */
    void _resize(size_t n);

    /*-------------------------------.
	| limb and bit accessor methods. |
	`-------------------------------*/

    /*! return number of limbs */
    size_t num_limbs() const;

    /*! return maximum number of limbs */
    size_t max_limbs() const;

    /*! access word at limb offset */
    ulimb_t limb_at(size_t n) const;

    /*! limb_mask at limb offset */
    ulimb_t limb_mask(size_t n) const;

    /*! test bit at bit offset */
    int test_bit(size_t n) const;

    /*! set bit at bit offset */
    void set_bit(size_t n);

    /*! return number of bits */
    size_t num_bits() const;

    /*! test sign */
    bool sign_bit() const;

    /*---------------------------------------------.
	| add, subtract, shifts and logical operators. |
	`---------------------------------------------*/

    /*! add with carry equals */
    bignum &operator+=(const bignum &operand);

    /*! subtract with borrow equals */
    bignum &operator-=(const bignum &operand);

    /*! left shift equals */
    bignum &operator<<=(size_t shamt);

    /*! right shift equals */
    bignum &operator>>=(size_t shamt);

    /*! bitwise and equals */
    bignum &operator&=(const bignum &operand);

    /*! bitwise or equals */
    bignum &operator|=(const bignum &operand);

    /*! bitwise xor equals */
    bignum &operator^=(const bignum &operand);

    /*! add with carry */
    bignum operator+(const bignum &operand) const;

    /*! subtract with borrow */
    bignum operator-(const bignum &operand) const;

    /*! left shift */
    bignum operator<<(size_t shamt) const;

    /*! right shift */
    bignum operator>>(size_t shamt) const;

    /*! bitwise and */
    bignum operator&(const bignum &operand) const;

    /*! bitwise or */
    bignum operator|(const bignum &operand) const;

    /*! bitwise xor */
    bignum operator^(const bignum &operand) const;

    /*! bitwise not */
    bignum operator~() const;

    /*! negate */
    bignum operator-() const;

    /*----------------------.
	| comparison operators. |
	`----------------------*/

    /*! equals */
    bool operator==(const bignum &operand) const;

    /*! less than */
    bool operator<(const bignum &operand) const;

    /*! not equals */
    bool operator!=(const bignum &operand) const;

    /*! less than or equal*/
    bool operator<=(const bignum &operand) const;

    /*! greater than */
    bool operator>(const bignum &operand) const;

    /*! less than or equal*/
    bool operator>=(const bignum &operand) const;

    /*! not */
    bool operator!() const;

    /*-------------------------.
	| multply, divide and pow. |
	`-------------------------*/

    /*! base 2^limb_bits multiply */
    static void mult(const bignum &multiplicand, const bignum multiplier, bignum &result);

    /*! base 2^limb_bits division */
    static void divrem(const bignum &dividend, const bignum &divisor, bignum &quotient, bignum &remainder);

    /*! multiply */
    bignum operator*(const bignum &operand) const;

    /*! division quotient */
    bignum operator/(const bignum &divisor) const;

    /*! division remainder */
    bignum operator%(const bignum &divisor) const;

    /*! multiply equals */
    bignum &operator*=(const bignum &operand);

    /*! divide equals */
    bignum &operator/=(const bignum &operand);

    /*! modulus equals */
    bignum &operator%=(const bignum &operand);

    /*! raise to the power */
    bignum pow(size_t exp) const;

    /*-------------------.
	| string conversion. |
	`-------------------*/

    /*! convert bignum to string */
    std::string to_string(size_t radix = 10) const;

    /*! convert bignum from string */
    void from_string(const char *str, size_t len, size_t radix);
};

/*------------------.
| inttype.          |
`------------------*/

struct inttype {
    bignum v;
    inttype() = delete;
    inttype(const bignum::ulimb_t n, const signedness s, const bitwidth b)
        : v(n, s, b) { }
    inttype(const std::initializer_list<bignum::ulimb_t> l, const signedness s, const bitwidth b)
        : v(l, s, b) { }
    operator const bignum &() const { return v; }
    inttype &operator=(const inttype &) = default;
};

struct Uint8 : inttype {
    Uint8(uint8_t v)
        : inttype(v, is_unsigned(), 8) { }
};
struct Sint8 : inttype {
    Sint8(int8_t v)
        : inttype(v, is_signed(), 8) { }
};
struct Uint16 : inttype {
    Uint16(uint16_t v)
        : inttype(v, is_unsigned(), 16) { }
};
struct Sint16 : inttype {
    Sint16(int16_t v)
        : inttype(v, is_signed(), 16) { }
};
struct Uint32 : inttype {
    Uint32(uint32_t v)
        : inttype(v, is_unsigned(), 32) { }
};
struct Sint32 : inttype {
    Sint32(int32_t v)
        : inttype(v, is_signed(), 32) { }
};
struct Uint64 : inttype {
    Uint64(uint64_t v)
        : inttype({ uint32_t(v), uint32_t(v >> 32) }, is_unsigned(), 64) { }
};
struct Sint64 : inttype {
    Sint64(int64_t v)
        : inttype({ uint32_t(v), uint32_t(v >> 32) }, is_signed(), 64) { }
};
