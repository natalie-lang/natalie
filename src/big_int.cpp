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

#include "natalie/big_int.hpp"
#include <tuple>

/*
    is_valid_number
    ---------------
    Checks whether the given string is a valid integer.
*/

bool is_valid_number(const Natalie::String &num) {
    for (size_t i = 0; i < num.size(); ++i) {
        char digit = num[i];
        if (digit < '0' or digit > '9')
            return false;
    }
    return true;
}

/*
    strip_leading_zeroes
    --------------------
    Strip the leading zeroes from a number represented as a string.
*/

void strip_leading_zeroes(Natalie::String &num) {
    size_t i;
    for (i = 0; i < num.size(); i++)
        if (num[i] != '0')
            break;

    if (i == num.size())
        num = "0";
    else
        num = num.substring(i);
}

/*
    add_leading_zeroes
    ------------------
    Adds a given number of leading zeroes to a string-represented integer `num`.
*/

void add_leading_zeroes(Natalie::String &num, size_t num_zeroes) {
    num.prepend(Natalie::String(num_zeroes, '0').c_str());
}

/*
    add_trailing_zeroes
    -------------------
    Adds a given number of trailing zeroes to a string-represented integer `num`.
*/

void add_trailing_zeroes(Natalie::String &num, size_t num_zeroes) {
    num.append(Natalie::String(num_zeroes, '0').c_str());
}

/*
    get_larger_and_smaller
    ----------------------
    Identifies the given string-represented integers as `larger` and `smaller`,
    padding the smaller number with leading zeroes to make it equal in length to
    the larger number.
*/

std::tuple<Natalie::String, Natalie::String> get_larger_and_smaller(const Natalie::String &num1,
    const Natalie::String &num2) {
    Natalie::String larger, smaller;
    if (num1.size() > num2.size() or (num1.size() == num2.size() and num1 > num2)) {
        larger = num1;
        smaller = num2;
    } else {
        larger = num2;
        smaller = num1;
    }

    // pad the smaller number with zeroes
    add_leading_zeroes(smaller, larger.size() - smaller.size());

    return std::make_tuple(larger, smaller);
}

/*
    is_power_of_10
    ----------------------
    Checks whether a string-represented integer is a power of 10.
*/

bool is_power_of_10(const Natalie::String &num) {
    if (num[0] != '1')
        return false;
    for (size_t i = 1; i < num.size(); i++)
        if (num[i] != '0')
            return false;

    return true; // first digit is 1 and the following digits are all 0
}

/*
    ===========================================================================
    Random number generating functions for BigInt
    ===========================================================================
*/

#include <climits>
#include <random>

// when the number of digits are not specified, a random value is used for it
// which is kept below the following:
const size_t MAX_RANDOM_LENGTH = 1000;

/*
    big_random (num_digits)
    -----------------------
    Returns a random BigInt with a specific number of digits.
*/

BigInt big_random(size_t num_digits = 0) {
    std::random_device rand_generator; // true random number generator

    if (num_digits == 0) // the number of digits were not specified
        // use a random number for it:
        num_digits = 1 + rand_generator() % MAX_RANDOM_LENGTH;

    BigInt big_rand;
    big_rand.value = ""; // clear value to append digits

    // ensure that the first digit is non-zero
    big_rand.value.append((size_t)(1 + rand_generator() % 9));

    while (big_rand.value.size() < num_digits)
        big_rand.value.append((size_t)rand_generator());
    if (big_rand.value.size() != num_digits)
        big_rand.value.truncate(num_digits); // erase extra digits

    return big_rand;
}

/*
    ===========================================================================
    Constructors
    ===========================================================================
*/

/*
    Default constructor
    -------------------
*/

BigInt::BigInt() {
    value = "0";
    sign = '+';
}

/*
    Copy constructor
    ----------------
*/

BigInt::BigInt(const BigInt &num) {
    value = num.value;
    sign = num.sign;
}

/*
    Integer to BigInt
    -----------------
*/

BigInt::BigInt(const long long &num) {
    value = Natalie::String(std::abs(num));
    if (num < 0)
        sign = '-';
    else
        sign = '+';
}

/*
    String to BigInt
    ----------------
*/

BigInt::BigInt(const Natalie::String &num) {
    if (num[0] == '+' or num[0] == '-') { // check for sign
        Natalie::String magnitude = num.substring(1);
        // Expected an integer, got num
        assert(is_valid_number(magnitude));
        value = magnitude;
        sign = num[0];
    } else { // if no sign is specified
        // Expected an integer, got num
        assert(is_valid_number(num));
        value = num;
        sign = '+'; // positive by default
    }
    strip_leading_zeroes(value);
}

/*
    ===========================================================================
    Conversion functions for BigInt
    ===========================================================================
*/

/*
    to_string
    ---------
    Converts a BigInt to a string.
*/

Natalie::String BigInt::to_string() const {
    // prefix with sign if negative
    if (this->sign == '-') {
        auto copy = this->value;
        copy.prepend_char(this->sign);
        return copy;
    } else {
        return this->value;
    }
}

/*
    to_long
    -------
    Converts a BigInt to a long int.
    NOTE: If the BigInt is out of range of a long int, stol() will throw an
    out_of_range exception.
*/

long BigInt::to_long() const {
    return strtol(this->to_string().c_str(), NULL, 2);
}

/*
    to_long_long
    ------------
    Converts a BigInt to a long long int.
    NOTE: If the BigInt is out of range of a long long int, stoll() will throw
    an out_of_range exception.
*/

long long BigInt::to_long_long() const {
    return strtoll(this->to_string().c_str(), NULL, 2);
}

/*
    ===========================================================================
    Assignment operators
    ===========================================================================
*/

/*
    BigInt = BigInt
    ---------------
*/

BigInt &BigInt::operator=(const BigInt &num) {
    value = num.value;
    sign = num.sign;

    return *this;
}

/*
    BigInt = Integer
    ----------------
*/

BigInt &BigInt::operator=(const long long &num) {
    BigInt temp(num);
    value = temp.value;
    sign = temp.sign;

    return *this;
}

/*
    BigInt = String
    ---------------
*/

BigInt &BigInt::operator=(const Natalie::String &num) {
    BigInt temp(num);
    value = temp.value;
    sign = temp.sign;

    return *this;
}

/*
    ===========================================================================
    Unary arithmetic operators
    ===========================================================================
*/

/*
    +BigInt
    -------
    Returns the value of a BigInt.
    NOTE: This function does not return the absolute value. To get the absolute
    value of a BigInt, use the `abs` function.
*/

BigInt BigInt::operator+() const {
    return *this;
}

/*
    -BigInt
    -------
    Returns the negative of a BigInt.
*/

BigInt BigInt::operator-() const {
    BigInt temp;

    temp.value = value;
    if (value != "0") {
        if (sign == '+')
            temp.sign = '-';
        else
            temp.sign = '+';
    }

    return temp;
}

/*
    ===========================================================================
    Relational operators
    ===========================================================================
    All operators depend on the '<' and/or '==' operator(s).
*/

/*
    BigInt == BigInt
    ----------------
*/

bool BigInt::operator==(const BigInt &num) const {
    return (sign == num.sign) and (value == num.value);
}

/*
    BigInt != BigInt
    ----------------
*/

bool BigInt::operator!=(const BigInt &num) const {
    return !(*this == num);
}

/*
    BigInt < BigInt
    ---------------
*/

bool BigInt::operator<(const BigInt &num) const {
    if (sign == num.sign) {
        if (sign == '+') {
            if (value.length() == num.value.length())
                return value < num.value;
            else
                return value.length() < num.value.length();
        } else
            return -(*this) > -num;
    } else
        return sign == '-';
}

/*
    BigInt > BigInt
    ---------------
*/

bool BigInt::operator>(const BigInt &num) const {
    return !((*this < num) or (*this == num));
}

/*
    BigInt <= BigInt
    ----------------
*/

bool BigInt::operator<=(const BigInt &num) const {
    return (*this < num) or (*this == num);
}

/*
    BigInt >= BigInt
    ----------------
*/

bool BigInt::operator>=(const BigInt &num) const {
    return !(*this < num);
}

/*
    BigInt == Integer
    -----------------
*/

bool BigInt::operator==(const long long &num) const {
    return *this == BigInt(num);
}

/*
    Integer == BigInt
    -----------------
*/

bool operator==(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) == rhs;
}

/*
    BigInt != Integer
    -----------------
*/

bool BigInt::operator!=(const long long &num) const {
    return !(*this == BigInt(num));
}

/*
    Integer != BigInt
    -----------------
*/

bool operator!=(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) != rhs;
}

/*
    BigInt < Integer
    ----------------
*/

bool BigInt::operator<(const long long &num) const {
    return *this < BigInt(num);
}

/*
    Integer < BigInt
    ----------------
*/

bool operator<(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) < rhs;
}

/*
    BigInt > Integer
    ----------------
*/

bool BigInt::operator>(const long long &num) const {
    return *this > BigInt(num);
}

/*
    Integer > BigInt
    ----------------
*/

bool operator>(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) > rhs;
}

/*
    BigInt <= Integer
    -----------------
*/

bool BigInt::operator<=(const long long &num) const {
    return !(*this > BigInt(num));
}

/*
    Integer <= BigInt
    -----------------
*/

bool operator<=(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) <= rhs;
}

/*
    BigInt >= Integer
    -----------------
*/

bool BigInt::operator>=(const long long &num) const {
    return !(*this < BigInt(num));
}

/*
    Integer >= BigInt
    -----------------
*/

bool operator>=(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) >= rhs;
}

/*
    BigInt == String
    ----------------
*/

bool BigInt::operator==(const Natalie::String &num) const {
    return *this == BigInt(num);
}

/*
    String == BigInt
    ----------------
*/

bool operator==(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) == rhs;
}

/*
    BigInt != String
    ----------------
*/

bool BigInt::operator!=(const Natalie::String &num) const {
    return !(*this == BigInt(num));
}

/*
    String != BigInt
    ----------------
*/

bool operator!=(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) != rhs;
}

/*
    BigInt < String
    ---------------
*/

bool BigInt::operator<(const Natalie::String &num) const {
    return *this < BigInt(num);
}

/*
    String < BigInt
    ---------------
*/

bool operator<(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) < rhs;
}

/*
    BigInt > String
    ---------------
*/

bool BigInt::operator>(const Natalie::String &num) const {
    return *this > BigInt(num);
}

/*
    String > BigInt
    ---------------
*/

bool operator>(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) > rhs;
}

/*
    BigInt <= String
    ----------------
*/

bool BigInt::operator<=(const Natalie::String &num) const {
    return !(*this > BigInt(num));
}

/*
    String <= BigInt
    ----------------
*/

bool operator<=(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) <= rhs;
}

/*
    BigInt >= String
    ----------------
*/

bool BigInt::operator>=(const Natalie::String &num) const {
    return !(*this < BigInt(num));
}

/*
    String >= BigInt
    ----------------
*/

bool operator>=(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) >= rhs;
}

/*
    ===========================================================================
    Math functions for BigInt
    ===========================================================================
*/

/*
    abs
    ---
    Returns the absolute value of a BigInt.
*/

BigInt abs(const BigInt &num) {
    return num < 0 ? -num : num;
}

/*
    big_pow10
    ---------
    Returns a BigInt equal to 10^exp.
    NOTE: exponent should be a non-negative integer.
*/

BigInt big_pow10(size_t exp) {
    auto string = Natalie::String(exp, '0');
    string.prepend_char('1');
    return BigInt(string);
}

/*
    pow (BigInt)
    ------------
    Returns a BigInt equal to base^exp.
*/

BigInt pow(const BigInt &base, int exp) {
    if (exp < 0) {
        // Cannot divide by zero
        assert(base != 0);
        return abs(base) == 1 ? base : 0;
    }
    if (exp == 0) {
        // Zero cannot be raised to zero
        assert(base != 0);
        return 1;
    }

    BigInt result = base, result_odd = 1;
    while (exp > 1) {
        if (exp % 2)
            result_odd *= result;
        result *= result;
        exp /= 2;
    }

    return result * result_odd;
}

/*
    pow (Integer)
    -------------
    Returns a BigInt equal to base^exp.
*/

BigInt pow(const long long &base, int exp) {
    return pow(BigInt(base), exp);
}

/*
    pow (String)
    ------------
    Returns a BigInt equal to base^exp.
*/

BigInt pow(const Natalie::String &base, int exp) {
    return pow(BigInt(base), exp);
}

/*
    sqrt
    ----
    Returns the positive integer square root of a BigInt using Newton's method.
    NOTE: the input must be non-negative.
*/

BigInt sqrt(const BigInt &num) {
    // Cannot compute square root of a negative integer
    assert(num >= 0);

    // Optimisations for small inputs:
    if (num == 0)
        return 0;
    else if (num < 4)
        return 1;
    else if (num < 9)
        return 2;
    else if (num < 16)
        return 3;

    BigInt sqrt_prev = -1;
    // The value for `sqrt_current` is chosen close to that of the actual
    // square root.
    // Since a number's square root has at least one less than half as many
    // digits as the number,
    //     sqrt_current = 10^(half_the_digits_in_num - 1)
    BigInt sqrt_current = big_pow10(num.to_string().size() / 2 - 1);

    while (abs(sqrt_current - sqrt_prev) > 1) {
        sqrt_prev = sqrt_current;
        sqrt_current = (num / sqrt_prev + sqrt_prev) / 2;
    }

    return sqrt_current;
}

/*
    gcd(BigInt, BigInt)
    -------------------
    Returns the greatest common divisor (GCD, a.k.a. HCF) of two BigInts using
    Euclid's algorithm.
*/

BigInt gcd(const BigInt &num1, const BigInt &num2) {
    BigInt abs_num1 = abs(num1);
    BigInt abs_num2 = abs(num2);

    // base cases:
    if (abs_num2 == 0)
        return abs_num1; // gcd(a, 0) = |a|
    if (abs_num1 == 0)
        return abs_num2; // gcd(0, a) = |a|

    BigInt remainder = abs_num2;
    while (remainder != 0) {
        remainder = abs_num1 % abs_num2;
        abs_num1 = abs_num2; // previous remainder
        abs_num2 = remainder; // current remainder
    }

    return abs_num1;
}

/*
    gcd(BigInt, Integer)
    --------------------
*/

BigInt gcd(const BigInt &num1, const long long &num2) {
    return gcd(num1, BigInt(num2));
}

/*
    gcd(BigInt, String)
    -------------------
*/

BigInt gcd(const BigInt &num1, const Natalie::String &num2) {
    return gcd(num1, BigInt(num2));
}

/*
    gcd(Integer, BigInt)
    --------------------
*/

BigInt gcd(const long long &num1, const BigInt &num2) {
    return gcd(BigInt(num1), num2);
}

/*
    gcd(String, BigInt)
    -------------------
*/

BigInt gcd(const Natalie::String &num1, const BigInt &num2) {
    return gcd(BigInt(num1), num2);
}

/*
    lcm(BigInt, BigInt)
    -------------------
    Returns the least common multiple (LCM) of two BigInts.
*/

BigInt lcm(const BigInt &num1, const BigInt &num2) {
    if (num1 == 0 or num2 == 0)
        return 0;

    return abs(num1 * num2) / gcd(num1, num2);
}

/*
    lcm(BigInt, Integer)
    --------------------
*/

BigInt lcm(const BigInt &num1, const long long &num2) {
    return lcm(num1, BigInt(num2));
}

/*
    lcm(BigInt, String)
    -------------------
*/

BigInt lcm(const BigInt &num1, const Natalie::String &num2) {
    return lcm(num1, BigInt(num2));
}

/*
    lcm(Integer, BigInt)
    --------------------
*/

BigInt lcm(const long long &num1, const BigInt &num2) {
    return lcm(BigInt(num1), num2);
}

/*
    lcm(String, BigInt)
    -------------------
*/

BigInt lcm(const Natalie::String &num1, const BigInt &num2) {
    return lcm(BigInt(num1), num2);
}

/*
    ===========================================================================
    Binary arithmetic operators
    ===========================================================================
*/

#include <climits>
#include <cmath>
#include <string>

const long long FLOOR_SQRT_LLONG_MAX = 3037000499;

/*
    BigInt + BigInt
    ---------------
    The operand on the RHS of the addition is `num`.
*/

BigInt BigInt::operator+(const BigInt &num) const {
    // if the operands are of opposite signs, perform subtraction
    if (this->sign == '+' and num.sign == '-') {
        BigInt rhs = num;
        rhs.sign = '+';
        return *this - rhs;
    } else if (this->sign == '-' and num.sign == '+') {
        BigInt lhs = *this;
        lhs.sign = '+';
        return -(lhs - num);
    }

    // identify the numbers as `larger` and `smaller`
    Natalie::String larger, smaller;
    std::tie(larger, smaller) = get_larger_and_smaller(this->value, num.value);

    BigInt result; // the resultant sum
    result.value = ""; // the value is cleared as the digits will be appended
    short carry = 0, sum;
    // add the two values
    for (long i = larger.size() - 1; i >= 0; i--) {
        sum = larger[i] - '0' + smaller[i] - '0' + carry;
        result.value.prepend(sum % 10);
        carry = sum / (short)10;
    }
    if (carry)
        result.value.prepend(carry);

    // if the operands are negative, the result is negative
    if (this->sign == '-' and result.value != "0")
        result.sign = '-';

    return result;
}

/*
    BigInt - BigInt
    ---------------
    The operand on the RHS of the subtraction is `num`.
*/

BigInt BigInt::operator-(const BigInt &num) const {
    // if the operands are of opposite signs, perform addition
    if (this->sign == '+' and num.sign == '-') {
        BigInt rhs = num;
        rhs.sign = '+';
        return *this + rhs;
    } else if (this->sign == '-' and num.sign == '+') {
        BigInt lhs = *this;
        lhs.sign = '+';
        return -(lhs + num);
    }

    BigInt result; // the resultant difference
    // identify the numbers as `larger` and `smaller`
    Natalie::String larger, smaller;
    if (abs(*this) > abs(num)) {
        larger = this->value;
        smaller = num.value;

        if (this->sign == '-') // -larger - -smaller = -result
            result.sign = '-';
    } else {
        larger = num.value;
        smaller = this->value;

        if (num.sign == '+') // smaller - larger = -result
            result.sign = '-';
    }
    // pad the smaller number with zeroes
    add_leading_zeroes(smaller, larger.size() - smaller.size());

    result.value = ""; // the value is cleared as the digits will be appended
    short difference;
    long i, j;
    // subtract the two values
    for (i = larger.size() - 1; i >= 0; i--) {
        difference = larger[i] - smaller[i];
        if (difference < 0) {
            for (j = i - 1; j >= 0; j--) {
                if (larger[j] != '0') {
                    larger[j]--; // borrow from the j-th digit
                    break;
                }
            }
            j++;
            while (j != i) {
                larger[j] = '9'; // add the borrow and take away 1
                j++;
            }
            difference += 10; // add the borrow
        }
        result.value.prepend(difference);
    }
    strip_leading_zeroes(result.value);

    // if the result is 0, set its sign as +
    if (result.value == "0")
        result.sign = '+';

    return result;
}

/*
    BigInt * BigInt
    ---------------
    Computes the product of two BigInts using Karatsuba's algorithm.
    The operand on the RHS of the product is `num`.
*/

BigInt BigInt::operator*(const BigInt &num) const {
    if (*this == 0 or num == 0)
        return BigInt(0);
    if (*this == 1)
        return num;
    if (num == 1)
        return *this;

    BigInt product;
    if (abs(*this) <= FLOOR_SQRT_LLONG_MAX and abs(num) <= FLOOR_SQRT_LLONG_MAX)
        product = strtoll(this->value.c_str(), NULL, 2) * strtoll(num.value.c_str(), NULL, 2);
    else if (is_power_of_10(this->value)) { // if LHS is a power of 10 do optimised operation
        product.value = num.value;
        product.value.append(this->value.substring(1));
    } else if (is_power_of_10(num.value)) { // if RHS is a power of 10 do optimised operation
        product.value = this->value;
        product.value.append(num.value.substring(1));
    } else {
        // identify the numbers as `larger` and `smaller`
        Natalie::String larger, smaller;
        std::tie(larger, smaller) = get_larger_and_smaller(this->value, num.value);

        size_t half_length = larger.size() / 2;
        auto half_length_ceil = (size_t)ceil(larger.size() / 2.0);

        BigInt num1_high, num1_low;
        num1_high = larger.substring(0, half_length);
        num1_low = larger.substring(half_length);

        BigInt num2_high, num2_low;
        num2_high = smaller.substring(0, half_length);
        num2_low = smaller.substring(half_length);

        strip_leading_zeroes(num1_high.value);
        strip_leading_zeroes(num1_low.value);
        strip_leading_zeroes(num2_high.value);
        strip_leading_zeroes(num2_low.value);

        BigInt prod_high, prod_mid, prod_low;
        prod_high = num1_high * num2_high;
        prod_low = num1_low * num2_low;
        prod_mid = (num1_high + num1_low) * (num2_high + num2_low)
            - prod_high - prod_low;

        add_trailing_zeroes(prod_high.value, 2 * half_length_ceil);
        add_trailing_zeroes(prod_mid.value, half_length_ceil);

        strip_leading_zeroes(prod_high.value);
        strip_leading_zeroes(prod_mid.value);
        strip_leading_zeroes(prod_low.value);

        product = prod_high + prod_mid + prod_low;
    }
    strip_leading_zeroes(product.value);

    if (this->sign == num.sign)
        product.sign = '+';
    else
        product.sign = '-';

    return product;
}

/*
    divide
    ------
    Helper function that returns the quotient and remainder on dividing the
    dividend by the divisor, when the divisor is 1 to 10 times the dividend.
*/

std::tuple<BigInt, BigInt> divide(const BigInt &dividend, const BigInt &divisor) {
    BigInt quotient, remainder, temp;

    temp = divisor;
    quotient = 1;
    while (temp < dividend) {
        quotient++;
        temp += divisor;
    }
    if (temp > dividend) {
        quotient--;
        remainder = dividend - (temp - divisor);
    }

    return std::make_tuple(quotient, remainder);
}

/*
    BigInt / BigInt
    ---------------
    Computes the quotient of two BigInts using the long-division method.
    The operand on the RHS of the division (the divisor) is `num`.
*/

BigInt BigInt::operator/(const BigInt &num) const {
    BigInt abs_dividend = abs(*this);
    BigInt abs_divisor = abs(num);

    // Attempted division by zero
    assert(num != 0);

    if (abs_dividend < abs_divisor)
        return BigInt(0);
    if (num == 1)
        return *this;
    if (num == -1)
        return -(*this);

    BigInt quotient;
    if (abs_dividend <= LLONG_MAX and abs_divisor <= LLONG_MAX)
        quotient = strtoll(abs_dividend.value.c_str(), NULL, 2) / strtoll(abs_divisor.value.c_str(), NULL, 2);
    else if (abs_dividend == abs_divisor)
        quotient = 1;
    else if (is_power_of_10(abs_divisor.value)) { // if divisor is a power of 10 do optimised calculation
        size_t digits_in_quotient = abs_dividend.value.size() - abs_divisor.value.size() + 1;
        quotient.value = abs_dividend.value.substring(0, digits_in_quotient);
    } else {
        quotient.value = ""; // the value is cleared as digits will be appended
        BigInt chunk, chunk_quotient, chunk_remainder;
        size_t chunk_index = 0;
        chunk_remainder.value = abs_dividend.value.substring(chunk_index, abs_divisor.value.size() - 1);
        chunk_index = abs_divisor.value.size() - 1;
        while (chunk_index < abs_dividend.value.size()) {
            chunk_remainder.value.append(1, abs_dividend.value[chunk_index]);
            chunk.value = chunk_remainder.value;
            chunk_index++;
            while (chunk < abs_divisor) {
                quotient.value.append_char('0');
                if (chunk_index < abs_dividend.value.size()) {
                    chunk.value.append(1, abs_dividend.value[chunk_index]);
                    chunk_index++;
                } else
                    break;
            }
            if (chunk == abs_divisor) {
                quotient.value.append_char('1');
                chunk_remainder = 0;
            } else if (chunk > abs_divisor) {
                strip_leading_zeroes(chunk.value);
                std::tie(chunk_quotient, chunk_remainder) = divide(chunk, abs_divisor);
                quotient.value.append(chunk_quotient.value.c_str());
            }
        }
    }
    strip_leading_zeroes(quotient.value);

    if (this->sign == num.sign)
        quotient.sign = '+';
    else
        quotient.sign = '-';

    return quotient;
}

/*
    BigInt % BigInt
    ---------------
    Computes the modulo (remainder on division) of two BigInts.
    The operand on the RHS of the modulo (the divisor) is `num`.
*/

BigInt BigInt::operator%(const BigInt &num) const {
    BigInt abs_dividend = abs(*this);
    BigInt abs_divisor = abs(num);

    // Attempted division by zero
    assert(abs_divisor != 0);
    if (abs_divisor == 1 or abs_divisor == abs_dividend)
        return BigInt(0);

    BigInt remainder;
    if (abs_dividend <= LLONG_MAX and abs_divisor <= LLONG_MAX)
        remainder = strtoll(abs_dividend.value.c_str(), NULL, 2) % strtoll(abs_divisor.value.c_str(), NULL, 2);
    else if (abs_dividend < abs_divisor)
        remainder = abs_dividend;
    else if (is_power_of_10(num.value)) { // if num is a power of 10 use optimised calculation
        size_t no_of_zeroes = num.value.size() - 1;
        remainder.value = abs_dividend.value.substring(abs_dividend.value.size() - no_of_zeroes);
    } else {
        BigInt quotient = abs_dividend / abs_divisor;
        remainder = abs_dividend - quotient * abs_divisor;
    }
    strip_leading_zeroes(remainder.value);

    // remainder has the same sign as that of the dividend
    remainder.sign = this->sign;
    if (remainder.value == "0") // except if its zero
        remainder.sign = '+';

    return remainder;
}

/*
    BigInt + Integer
    ----------------
*/

BigInt BigInt::operator+(const long long &num) const {
    return *this + BigInt(num);
}

/*
    Integer + BigInt
    ----------------
*/

BigInt operator+(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) + rhs;
}

/*
    BigInt - Integer
    ----------------
*/

BigInt BigInt::operator-(const long long &num) const {
    return *this - BigInt(num);
}

/*
    Integer - BigInt
    ----------------
*/

BigInt operator-(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) - rhs;
}

/*
    BigInt * Integer
    ----------------
*/

BigInt BigInt::operator*(const long long &num) const {
    return *this * BigInt(num);
}

/*
    Integer * BigInt
    ----------------
*/

BigInt operator*(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) * rhs;
}

/*
    BigInt / Integer
    ----------------
*/

BigInt BigInt::operator/(const long long &num) const {
    return *this / BigInt(num);
}

/*
    Integer / BigInt
    ----------------
*/

BigInt operator/(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) / rhs;
}

/*
    BigInt % Integer
    ----------------
*/

BigInt BigInt::operator%(const long long &num) const {
    return *this % BigInt(num);
}

/*
    Integer % BigInt
    ----------------
*/

BigInt operator%(const long long &lhs, const BigInt &rhs) {
    return BigInt(lhs) % rhs;
}

/*
    BigInt + String
    ---------------
*/

BigInt BigInt::operator+(const Natalie::String &num) const {
    return *this + BigInt(num);
}

/*
    String + BigInt
    ---------------
*/

BigInt operator+(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) + rhs;
}

/*
    BigInt - String
    ---------------
*/

BigInt BigInt::operator-(const Natalie::String &num) const {
    return *this - BigInt(num);
}

/*
    String - BigInt
    ---------------
*/

BigInt operator-(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) - rhs;
}

/*
    BigInt * String
    ---------------
*/

BigInt BigInt::operator*(const Natalie::String &num) const {
    return *this * BigInt(num);
}

/*
    String * BigInt
    ---------------
*/

BigInt operator*(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) * rhs;
}

/*
    BigInt / String
    ---------------
*/

BigInt BigInt::operator/(const Natalie::String &num) const {
    return *this / BigInt(num);
}

/*
    String / BigInt
    ---------------
*/

BigInt operator/(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) / rhs;
}

/*
    BigInt % String
    ---------------
*/

BigInt BigInt::operator%(const Natalie::String &num) const {
    return *this % BigInt(num);
}

/*
    String % BigInt
    ---------------
*/

BigInt operator%(const Natalie::String &lhs, const BigInt &rhs) {
    return BigInt(lhs) % rhs;
}

/*
    ===========================================================================
    Arithmetic-assignment operators
    ===========================================================================
*/

/*
    BigInt += BigInt
    ----------------
*/

BigInt &BigInt::operator+=(const BigInt &num) {
    *this = *this + num;

    return *this;
}

/*
    BigInt -= BigInt
    ----------------
*/

BigInt &BigInt::operator-=(const BigInt &num) {
    *this = *this - num;

    return *this;
}

/*
    BigInt *= BigInt
    ----------------
*/

BigInt &BigInt::operator*=(const BigInt &num) {
    *this = *this * num;

    return *this;
}

/*
    BigInt /= BigInt
    ----------------
*/

BigInt &BigInt::operator/=(const BigInt &num) {
    *this = *this / num;

    return *this;
}

/*
    BigInt %= BigInt
    ----------------
*/

BigInt &BigInt::operator%=(const BigInt &num) {
    *this = *this % num;

    return *this;
}

/*
    BigInt += Integer
    -----------------
*/

BigInt &BigInt::operator+=(const long long &num) {
    *this = *this + BigInt(num);

    return *this;
}

/*
    BigInt -= Integer
    -----------------
*/

BigInt &BigInt::operator-=(const long long &num) {
    *this = *this - BigInt(num);

    return *this;
}

/*
    BigInt *= Integer
    -----------------
*/

BigInt &BigInt::operator*=(const long long &num) {
    *this = *this * BigInt(num);

    return *this;
}

/*
    BigInt /= Integer
    -----------------
*/

BigInt &BigInt::operator/=(const long long &num) {
    *this = *this / BigInt(num);

    return *this;
}

/*
    BigInt %= Integer
    -----------------
*/

BigInt &BigInt::operator%=(const long long &num) {
    *this = *this % BigInt(num);

    return *this;
}

/*
    BigInt += String
    ----------------
*/

BigInt &BigInt::operator+=(const Natalie::String &num) {
    *this = *this + BigInt(num);

    return *this;
}

/*
    BigInt -= String
    ----------------
*/

BigInt &BigInt::operator-=(const Natalie::String &num) {
    *this = *this - BigInt(num);

    return *this;
}

/*
    BigInt *= String
    ----------------
*/

BigInt &BigInt::operator*=(const Natalie::String &num) {
    *this = *this * BigInt(num);

    return *this;
}

/*
    BigInt /= String
    ----------------
*/

BigInt &BigInt::operator/=(const Natalie::String &num) {
    *this = *this / BigInt(num);

    return *this;
}

/*
    BigInt %= String
    ----------------
*/

BigInt &BigInt::operator%=(const Natalie::String &num) {
    *this = *this % BigInt(num);

    return *this;
}

/*
    ===========================================================================
    Increment and decrement operators
    ===========================================================================
*/

/*
    Pre-increment
    -------------
    ++BigInt
*/

BigInt &BigInt::operator++() {
    *this += 1;

    return *this;
}

/*
    Pre-decrement
    -------------
    --BigInt
*/

BigInt &BigInt::operator--() {
    *this -= 1;

    return *this;
}

/*
    Post-increment
    --------------
    BigInt++
*/

BigInt BigInt::operator++(int) {
    BigInt temp = *this;
    *this += 1;

    return temp;
}

/*
    Post-decrement
    --------------
    BigInt--
*/

BigInt BigInt::operator--(int) {
    BigInt temp = *this;
    *this -= 1;

    return temp;
}
