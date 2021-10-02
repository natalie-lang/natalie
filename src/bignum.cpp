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

#include <cassert>
#include <cstddef>

#include "natalie/bignum/bignum.hpp"

using ulimb_t = bignum::ulimb_t;
using udlimb_t = bignum::udlimb_t;

/*--------------.
| constructors. |
`--------------*/

/*! empty constructor */
bignum::bignum(const signedness s, const bitwidth bits)
    : limbs { 0 }
    , s(s)
    , bits(bits) { }

/*! integral constructor */
bignum::bignum(const ulimb_t n, const signedness s, const bitwidth bits)
    : limbs { n }
    , s(s)
    , bits(bits) {
    _contract();
}

/*! array constructor */
bignum::bignum(const std::initializer_list<ulimb_t> l, const signedness s, const bitwidth bits)
    : limbs(l)
    , s(s)
    , bits(bits) {
    _contract();
}

/*! string constructor */
bignum::bignum(std::string str, const signedness s, const bitwidth bits)
    : limbs { 0 }
    , s(s)
    , bits(bits) {
    from_string(str.c_str(), str.size(), 0);
    this->s = s;
    this->bits = bits;
    _contract();
}

/*! string constructor with radix */
bignum::bignum(std::string str, const size_t radix, const signedness s, const bitwidth bits)
    : limbs { 0 }
    , s(s)
    , bits(bits) {
    from_string(str.c_str(), str.size(), radix);
    this->s = s;
    this->bits = bits;
    _contract();
}

/*! copy constructor  */
bignum::bignum(const bignum &operand)
    : limbs(operand.limbs)
    , s(operand.s)
    , bits(operand.bits) {
    _contract();
}

/*! move constructor  */
bignum::bignum(const bignum &&operand) noexcept
    : limbs(std::move(operand.limbs))
    , s(operand.s)
    , bits(operand.bits) {
    _contract();
}

/*----------------------.
| assignment operators. |
`----------------------*/

/*! integral copy assignment operator */
bignum &bignum::operator=(const ulimb_t l) {
    _resize(1);
    limbs[0] = l;
    _contract();
    return *this;
}

/*! bignum copy assignment operator */
bignum &bignum::operator=(const bignum &operand) {
    limbs = operand.limbs;
    if (bits == 0) bits = operand.bits;
    s = operand.s;
    return *this;
}

/*! bignum move assignment operator */
bignum &bignum::operator=(bignum &&operand) {
    limbs = std::move(operand.limbs);
    if (bits == 0) bits = operand.bits;
    s = operand.s;
    return *this;
}

/*------------------.
| internal methods. |
`------------------*/

/*! expand limbs to match operand */
void bignum::_expand(const bignum &operand) {
    limbs.resize(std::min(max_limbs(), std::max(num_limbs(), operand.num_limbs())));
}

/*! contract zero big end limbs */
void bignum::_contract() {
    while (bits > 0 && num_limbs() > max_limbs()) {
        limbs.pop_back();
    }
    while (num_limbs() > 1 && limbs.back() == 0) {
        limbs.pop_back();
    }
    if (bits > 0 && num_limbs() == max_limbs()) {
        limbs.back() &= limb_mask(num_limbs() - 1);
    }
}

/*! resize number of limbs */
void bignum::_resize(size_t n) {
    limbs.resize(n);
}

/*-------------------------------.
| limb and bit accessor methods. |
`-------------------------------*/

/*! return number of limbs */
size_t bignum::num_limbs() const { return limbs.size(); }

/*! return maximum number of limbs */
size_t bignum::max_limbs() const { return ((bits - 1) >> limb_shift) + 1; }

/*! access word at limb offset */
ulimb_t bignum::limb_at(size_t n) const { return n < num_limbs() ? limbs[n] : 0; }

/*! limb_mask at limb offset */
ulimb_t bignum::limb_mask(size_t n) const {
    if (bits == 0) return -1;
    if (n < (bits >> limb_shift)) return -1;
    if (n > (bits >> limb_shift))
        return 0;
    else
        return ((1 << (bits & ((1ULL << limb_bits) - 1))) - 1);
}

/*! test bit at bit offset */
int bignum::test_bit(size_t n) const {
    size_t word = n >> limb_shift;
    if (word < num_limbs())
        return (limbs[word] >> (n & (limb_bits - 1))) & 1;
    else
        return 0;
}

/*! set bit at bit offset */
void bignum::set_bit(size_t n) {
    size_t word = n >> limb_shift;
    if (word >= num_limbs()) _resize(word + 1);
    limbs[word] |= (1ULL << (n & (limb_bits - 1)));
}

/*! return number of bits */
size_t bignum::num_bits() const {
    if (bits > 0) return bits;
    if (limbs.size() == 1 && limbs[0] == 0) return 0;
    return (limb_bits - clz(limbs.back())) + (num_limbs() - 1) * limb_bits;
}

/*! test sign */
bool bignum::sign_bit() const {
    return s.is_signed && bits > 0 ? test_bit(bits - 1) : 0;
}

/*---------------------.
| mutating operations. |
`---------------------*/

/*! add with carry equals */
bignum &bignum::operator+=(const bignum &operand) {
    _expand(operand);
    ulimb_t carry = 0;
    for (size_t i = 0; i < num_limbs(); i++) {
        ulimb_t old_val = limbs[i];
        ulimb_t new_val = old_val + operand.limb_at(i) + carry;
        limbs[i] = new_val;
        carry = new_val < old_val;
    }
    if (carry && num_limbs() < max_limbs()) {
        limbs.push_back(1);
    }
    return *this;
}

/*! subtract with borrow equals */
bignum &bignum::operator-=(const bignum &operand) {
    _expand(operand);
    ulimb_t borrow = 0;
    for (size_t i = 0; i < num_limbs(); i++) {
        ulimb_t old_val = limbs[i];
        ulimb_t new_val = old_val - operand.limb_at(i) - borrow;
        limbs[i] = new_val;
        borrow = new_val > old_val;
    }
    _contract();
    return *this;
}

/*! left shift equals */
bignum &bignum::operator<<=(size_t shamt) {
    size_t limb_shamt = shamt >> limb_shift;
    if (limb_shamt > 0) {
        limbs.insert(limbs.begin(), std::min(max_limbs(), limb_shamt), 0);
        shamt -= (limb_shamt << limb_shift);
    }
    _contract();
    if (!shamt) return *this;

    ulimb_t carry = 0;
    for (size_t j = 0; j < num_limbs(); j++) {
        ulimb_t old_val = limbs[j];
        ulimb_t new_val = (old_val << shamt) | carry;
        limbs[j] = new_val;
        carry = old_val >> (limb_bits - shamt);
    }
    if (carry && num_limbs() < max_limbs()) {
        limbs.push_back(carry);
    }
    _contract();
    return *this;
}

/*! right shift equals */
bignum &bignum::operator>>=(size_t shamt) {
    size_t limb_shamt = shamt >> limb_shift;
    if (limb_shamt > 0) {
        limbs.erase(limbs.begin(), limbs.begin() + std::min(num_limbs(), limb_shamt));
        shamt -= (limb_shamt << limb_shift);
    }
    if (num_limbs() == 0) {
        *this = 0;
    }
    if (!shamt) return *this;

    ulimb_t carry = -(s.is_signed && sign_bit()) << (shamt - 1);
    for (size_t j = num_limbs(); j > 0; j--) {
        ulimb_t old_val = limbs[j - 1];
        ulimb_t new_val = (old_val >> shamt) | carry;
        limbs[j - 1] = new_val;
        carry = old_val << (limb_bits - shamt);
    }
    _contract();
    return *this;
}

/*! bitwise and equals */
bignum &bignum::operator&=(const bignum &operand) {
    _expand(operand);
    for (size_t i = 0; i < num_limbs(); i++) {
        limbs[i] = operand.limb_at(i) & limbs[i];
    }
    _contract();
    return *this;
}

/*! bitwise or equals */
bignum &bignum::operator|=(const bignum &operand) {
    _expand(operand);
    for (size_t i = 0; i < num_limbs(); i++) {
        limbs[i] = operand.limb_at(i) | limbs[i];
    }
    _contract();
    return *this;
}

/*! bitwise xor equals */
bignum &bignum::operator^=(const bignum &operand) {
    _expand(operand);
    for (size_t i = 0; i < num_limbs(); i++) {
        limbs[i] = operand.limb_at(i) ^ limbs[i];
    }
    _contract();
    return *this;
}

/*------------------.
| const operations. |
`------------------*/

/* const operations copy and use the mutating operations */

/*! add with carry */
bignum bignum::operator+(const bignum &operand) const {
    bignum result(*this);
    return result += operand;
}

/*! subtract with borrow */
bignum bignum::operator-(const bignum &operand) const {
    bignum result(*this);
    return result -= operand;
}

/*! left shift */
bignum bignum::operator<<(size_t shamt) const {
    bignum result(*this);
    return result <<= shamt;
}

/*! right shift */
bignum bignum::operator>>(size_t shamt) const {
    bignum result(*this);
    return result >>= shamt;
}

/*! bitwise and */
bignum bignum::operator&(const bignum &operand) const {
    bignum result(*this);
    return result &= operand;
}

/*! bitwise or */
bignum bignum::operator|(const bignum &operand) const {
    bignum result(*this);
    return result |= operand;
}

/*! bitwise xor */
bignum bignum::operator^(const bignum &operand) const {
    bignum result(*this);
    return result ^= operand;
}

/*! bitwise not */
bignum bignum::operator~() const {
    bignum result(*this);
    for (auto &n : result.limbs) {
        n = ~n;
    }
    return result;
}

/*! negate */
bignum bignum::operator-() const {
    bignum result(*this);
    if (bits == 0) {
        return result;
    }
    size_t n = std::max(max_limbs(), num_limbs());
    result._resize(n);
    for (size_t i = 0; i < result.num_limbs(); i++) {
        result.limbs[i] = ~limb_at(i);
    }
    result += 1;
    result._contract();
    return result;
}

/*----------------------.
| comparison operators. |
`----------------------*/

/* comparison are defined in terms of "equals" and "less than" */

/*! equals */
bool bignum::operator==(const bignum &operand) const {
    if (num_limbs() != operand.num_limbs()) return false;
    for (size_t i = 0; i < num_limbs(); i++) {
        if (limbs[i] != operand.limbs[i]) return false;
    }
    return true;
}

/*! less than */
bool bignum::operator<(const bignum &operand) const {
    /* handle signed comparison if both operands are signed */
    if (bits > 0 && s.is_signed && operand.s.is_signed) {
        bool sign = sign_bit();
        if (sign ^ operand.sign_bit()) {
            return sign;
        } else if (sign) {
            return operand < *this;
        }
    }

    /* unsigned comparison */
    if (num_limbs() > operand.num_limbs())
        return false;
    else if (num_limbs() < operand.num_limbs())
        return true;
    for (ptrdiff_t i = num_limbs() - 1; i >= 0; i--) {
        if (limbs[i] > operand.limbs[i])
            return false;
        else if (limbs[i] < operand.limbs[i])
            return true;
    }
    return false;
}

/* axiomatically define other comparisons in terms of "equals" and "less than" */

/*! not equals */
bool bignum::operator!=(const bignum &operand) const { return !(*this == operand); }

/*! less than or equal*/
bool bignum::operator<=(const bignum &operand) const { return *this < operand || *this == operand; }

/*! greater than */
bool bignum::operator>(const bignum &operand) const { return !(*this <= operand); }

/*! less than or equal*/
bool bignum::operator>=(const bignum &operand) const { return !(*this < operand) || *this == operand; }

/*! not */
bool bignum::operator!() const { return *this == 0; }

/*--------------------.
| multply and divide. |
`--------------------*/

/*! base 2^limb_bits multiply */
void bignum::mult(const bignum &multiplicand, const bignum multiplier, bignum &result) {
    /* This routine is derived from Hacker's Delight,
	 * and possibly originates from Knuth */

    size_t m = multiplicand.num_limbs(), n = multiplier.num_limbs();
    size_t k = std::min(multiplicand.max_limbs(), m + n);
    result._resize(k);
    ulimb_t carry = 0;
    udlimb_t mj = multiplier.limbs[0];
    for (size_t i = 0; i < m && i < k; i++) {
        udlimb_t t = udlimb_t(multiplicand.limbs[i]) * mj + carry;
        result.limbs[i] = ulimb_t(t);
        carry = t >> limb_bits;
    }
    if (m < k) {
        result.limbs[m] = carry;
    }
    for (size_t j = 1; j < n; j++) {
        carry = 0;
        mj = multiplier.limbs[j];
        for (size_t i = 0; i < m && i + j < k; i++) {
            udlimb_t t = udlimb_t(multiplicand.limbs[i]) * mj + udlimb_t(result.limbs[i + j]) + carry;
            result.limbs[i + j] = ulimb_t(t);
            carry = t >> limb_bits;
        }
        if (j + m < k) {
            result.limbs[j + m] = carry;
        }
    }
    result._contract();
}

/*! base 2^limb_bits division */
void bignum::divrem(const bignum &dividend, const bignum &divisor, bignum &quotient, bignum &remainder) {
    /* This routine is derived from Hacker's Delight,
	 * and possibly originates from Knuth */

    quotient = 0;
    remainder = 0;
    ptrdiff_t m = dividend.num_limbs(), n = divisor.num_limbs();
    quotient._resize(std::max(m - n + 1, ptrdiff_t(1)));
    remainder._resize(n);
    ulimb_t *q = quotient.limbs.data(), *r = remainder.limbs.data();
    const ulimb_t *u = dividend.limbs.data(), *v = divisor.limbs.data();

    const udlimb_t b = (1ULL << limb_bits); // Number base
    ulimb_t *un, *vn; // Normalized form of u, v.
    udlimb_t qhat; // Estimated quotient digit.
    udlimb_t rhat; // A remainder.

    if (m < n || n <= 0 || v[n - 1] == 0) {
        quotient = 0;
        remainder = dividend;
        return;
    }

    // Single digit divisor
    if (n == 1) {
        udlimb_t k = 0;
        for (ptrdiff_t j = m - 1; j >= 0; j--) {
            q[j] = ulimb_t((k * b + u[j]) / v[0]);
            k = (k * b + u[j]) - q[j] * v[0];
        }
        r[0] = ulimb_t(k);
        quotient._contract();
        remainder._contract();
        return;
    }

    // Normalize by shifting v left just enough so that
    // its high-order bit is on, and shift u left the
    // same amount. We may have to append a high-order
    // digit on the dividend; we do that unconditionally.

    int s = clz(v[n - 1]); // 0 <= s <= limb_bits.
    vn = (ulimb_t *)alloca(sizeof(ulimb_t) * n);
    for (ptrdiff_t i = n - 1; i > 0; i--) {
        vn[i] = (v[i] << s) | (v[i - 1] >> (limb_bits - s));
    }
    vn[0] = v[0] << s;

    un = (ulimb_t *)alloca(sizeof(ulimb_t) * (m + 1));
    un[m] = u[m - 1] >> (limb_bits - s);
    for (ptrdiff_t i = m - 1; i > 0; i--) {
        un[i] = (u[i] << s) | (u[i - 1] >> (limb_bits - s));
    }
    un[0] = u[0] << s;
    for (ptrdiff_t j = m - n; j >= 0; j--) { // Main loop.
        // Compute estimate qhat of q[j].
        qhat = (un[j + n] * b + un[j + n - 1]) / vn[n - 1];
        rhat = (un[j + n] * b + un[j + n - 1]) - qhat * vn[n - 1];
    again:
        if (qhat >= b || qhat * vn[n - 2] > b * rhat + un[j + n - 2]) {
            qhat = qhat - 1;
            rhat = rhat + vn[n - 1];
            if (rhat < b) goto again;
        }
        // Multiply and subtract.
        udlimb_t k = 0;
        sdlimb_t t = 0;
        for (ptrdiff_t i = 0; i < n; i++) {
            unsigned long long p = qhat * vn[i];
            t = un[i + j] - k - (p & ((1ULL << limb_bits) - 1));
            un[i + j] = ulimb_t(t);
            k = (p >> limb_bits) - (t >> limb_bits);
        }
        t = un[j + n] - k;
        un[j + n] = ulimb_t(t);

        q[j] = ulimb_t(qhat); // Store quotient digit.
        if (t < 0) { // If we subtracted too
            q[j] = q[j] - 1; // much, add back.
            k = 0;
            for (ptrdiff_t i = 0; i < n; i++) {
                t = un[i + j] + vn[i] + k;
                un[i + j] = ulimb_t(t);
                k = t >> limb_bits;
            }
            un[j + n] = ulimb_t(un[j + n] + k);
        }
    }

    // normalize remainder
    for (ptrdiff_t i = 0; i < n; i++) {
        r[i] = (un[i] >> s) | (un[i + 1] << (limb_bits - s));
    }

    quotient._contract();
    remainder._contract();
}

/*! multiply */
bignum bignum::operator*(const bignum &operand) const {
    bignum result(0, s, bits);
    mult(*this, operand, result);
    result._contract();
    return result;
}

/*! division quotient */
bignum bignum::operator/(const bignum &divisor) const {
    bignum quotient(0, s, bits), remainder(0, s, bits);
    divrem(*this, divisor, quotient, remainder);
    return quotient;
}

/*! division remainder */
bignum bignum::operator%(const bignum &divisor) const {
    bignum quotient(0), remainder(0);
    divrem(*this, divisor, quotient, remainder);
    return remainder;
}

/*! multiply equals */
bignum &bignum::operator*=(const bignum &operand) {
    bignum result = *this * operand;
    *this = std::move(result);
    return *this;
}

/*! divide equals */
bignum &bignum::operator/=(const bignum &operand) {
    bignum result = *this / operand;
    *this = std::move(result);
    return *this;
}

/*! modulus equals */
bignum &bignum::operator%=(const bignum &operand) {
    bignum result = *this % operand;
    *this = std::move(result);
    return *this;
}

/*--------------------.
| power via squaring. |
`--------------------*/

/*! raise to the power */
bignum bignum::pow(size_t exp) const {
    if (exp == 0) return 1;
    bignum x = *this, y = 1;
    while (exp > 1) {
        if ((exp & 1) == 0) {
            exp >>= 1;
        } else {
            y *= x;
            exp = (exp - 1) >> 1;
        }
        x *= x;
    }
    return x * y;
}

/*-------------------.
| string conversion. |
`-------------------*/

/*! helper for recursive divide and conquer conversion to string */
static inline ptrdiff_t _to_string_c(const bignum &val, std::string &s, ptrdiff_t offset) {
    udlimb_t v = udlimb_t(val.limb_at(0)) | (udlimb_t(val.limb_at(1)) << bignum::limb_bits);
    do {
        s[--offset] = '0' + char(v % 10);
    } while ((v /= 10) != 0);
    return offset;
}

/*! helper for recursive divide and conquer conversion to string */
static ptrdiff_t _to_string_r(const bignum &val, std::vector<bignum> &sq, size_t level,
    std::string &s, size_t digits, ptrdiff_t offset) {
    bignum q, r;
    bignum::divrem(val, sq[level], q, r);
    if (level > 0) {
        if (r != 0) {
            if (q != 0) {
                _to_string_r(r, sq, level - 1, s, digits >> 1, offset);
                return _to_string_r(q, sq, level - 1, s, digits >> 1, offset - digits);
            } else {
                return _to_string_r(r, sq, level - 1, s, digits >> 1, offset);
            }
        }
    } else {
        if (r != 0) {
            if (q != 0) {
                _to_string_c(r, s, offset);
                offset = _to_string_c(q, s, offset - digits);
            } else {
                offset = _to_string_c(r, s, offset);
            }
        }
    }
    return offset;
}

/*! convert from bignum to string */
std::string bignum::to_string(size_t radix) const {
    static const char *hexdigits = "0123456789abcdef";
    static const bignum tenp18 { 0xa7640000, 0xde0b6b3 };
    static const size_t dgib = 3566893131; /* log2(10) * 1024^3 */

    switch (radix) {
    case 10: {
        if (*this == 0) return "0";

        /* estimate string length */
        std::string s;
        size_t climit = (size_t)(((long long)(num_limbs()) << (limb_shift + 30)) / (long long)dgib) + 1;
        s.resize(climit, '0');

        /* square the chunk size until ~= sqrt(n) */
        bignum chunk = tenp18;
        size_t digits = 18;
        std::vector<bignum> sq = { tenp18 };
        do {
            chunk *= chunk;
            digits <<= 1;
            sq.push_back(chunk);
        } while ((chunk.num_limbs() < ((num_limbs() >> 1) + 1)));

        /* recursively divide by chunk squares */
        ptrdiff_t offset = _to_string_r(*this, sq, sq.size() - 1, s, digits, climit);

        /* return less reserve */
        return s.substr(offset);
    }
    case 2: {
        if (*this == 0) return "0b0";

        std::string s("0b");
        ulimb_t l1 = limbs.back();
        size_t n = limb_bits - clz(l1);
        size_t t = n + ((num_limbs() - 1) << limb_shift);
        s.resize(t + 2);
        auto i = s.begin() + 2;
        for (ptrdiff_t k = n - 1; k >= 0; k--) {
            *(i++) = '0' + ((l1 >> k) & 1);
        }
        for (ptrdiff_t j = num_limbs() - 2; j >= 0; j--) {
            ulimb_t l = limbs[j];
            for (ptrdiff_t k = limb_bits - 1; k >= 0; k--) {
                *(i++) = '0' + ((l >> k) & 1);
            }
        }
        return s;
    }
    case 16: {
        if (*this == 0) return "0x0";

        std::string s("0x");
        ulimb_t l1 = limbs.back();
        size_t n = ((limb_bits >> 2) - (clz(l1) >> 2));
        size_t t = n + ((num_limbs() - 1) << (limb_shift - 2));
        s.resize(t + 2);
        auto i = s.begin() + 2;
        for (ptrdiff_t k = n - 1; k >= 0; k--) {
            *(i++) = hexdigits[(l1 >> (k << 2)) & 0xf];
        }
        for (ptrdiff_t j = num_limbs() - 2; j >= 0; j--) {
            ulimb_t l = limbs[j];
            for (ptrdiff_t k = (limb_bits >> 2) - 1; k >= 0; k--) {
                *(i++) = hexdigits[(l >> (k << 2)) & 0xf];
            }
        }
        return s;
    }
    default: {
        return std::string();
    }
    }
}

/*! convert to bignum from string */
void bignum::from_string(const char *str, size_t len, size_t radix) {
    static const bignum tenp18 { 0xa7640000, 0xde0b6b3 };
    static const bignum twop64 { 0, 0, 1 };
    if (len > 2) {
        if (strncmp(str, "0b", 2) == 0) {
            radix = 2;
            str += 2;
            len -= 2;
        } else if (strncmp(str, "0x", 2) == 0) {
            radix = 16;
            str += 2;
            len -= 2;
        }
    }
    if (radix == 0) {
        radix = 10;
    }
    switch (radix) {
    case 10: {
        for (size_t i = 0; i < len; i += 18) {
            size_t chunklen = i + 18 < len ? 18 : len - i;
            std::string chunk(str + i, chunklen);
            udlimb_t num = strtoull(chunk.c_str(), nullptr, 10);
            if (chunklen == 18) {
                *this *= tenp18;
            } else {
                *this *= bignum(10).pow(chunklen);
            }
            *this += bignum { ulimb_t(num), ulimb_t(num >> limb_bits) };
        }
        break;
    }
    case 2: {
        for (size_t i = 0; i < len; i += 64) {
            size_t chunklen = i + 64 < len ? 64 : len - i;
            std::string chunk(str + i, chunklen);
            udlimb_t num = strtoull(chunk.c_str(), nullptr, 2);
            if (chunklen == 64) {
                *this *= twop64;
            } else {
                *this *= bignum(2).pow(chunklen);
            }
            *this += bignum { ulimb_t(num), ulimb_t(num >> limb_bits) };
        }
        break;
    }
    case 16: {
        for (size_t i = 0; i < len; i += 16) {
            size_t chunklen = i + 16 < len ? 16 : len - i;
            std::string chunk(str + i, chunklen);
            udlimb_t num = strtoull(chunk.c_str(), nullptr, 16);
            if (chunklen == 16) {
                *this *= twop64;
            } else {
                *this *= bignum(16).pow(chunklen);
            }
            *this += bignum { ulimb_t(num), ulimb_t(num >> limb_bits) };
        }
        break;
    }
    default: {
        limbs.push_back(0);
    }
    }
}
