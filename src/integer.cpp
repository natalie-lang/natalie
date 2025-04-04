#include "natalie/integer.hpp"
#include "natalie/constants.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

Integer::Integer(double other) {
    assert(other == ::floor(other));
    auto i = (nat_int_t)other;
    if (i >= NAT_MIN_FIXNUM && i <= NAT_MAX_FIXNUM)
        m_value = left_shift_with_undefined_behavior(i, 1) | 0x1;
    else
        m_value = (uintptr_t) new BigInt(other);
}

Integer::Integer(const TM::String &other) {
    auto bigint = BigInt(other);
    if (bigint >= NAT_MIN_FIXNUM && bigint <= NAT_MAX_FIXNUM)
        m_value = left_shift_with_undefined_behavior(bigint.to_long_long(), 1) | 0x1;
    else
        m_value = (uintptr_t) new BigInt(bigint);
}

Integer::Integer(const BigInt &other) {
    if (other >= NAT_MIN_FIXNUM && other <= NAT_MAX_FIXNUM)
        m_value = left_shift_with_undefined_behavior(other.to_long_long(), 1) | 0x1;
    else
        m_value = (uintptr_t) new BigInt(other);
}

Integer::Integer(BigInt &&other) {
    if (other >= NAT_MIN_FIXNUM && other <= NAT_MAX_FIXNUM)
        m_value = left_shift_with_undefined_behavior(other.to_long_long(), 1) | 0x1;
    else
        m_value = (uintptr_t) new BigInt(std::move(other));
}

Integer Integer::operator+=(const Integer other) {
    *this = *this + other;
    return *this;
}

Integer Integer::operator-=(const Integer other) {
    *this = *this - other;
    return *this;
}

Integer Integer::operator*=(const Integer other) {
    *this = *this * other;
    return *this;
}

Integer Integer::operator/=(const Integer other) {
    *this = *this / other;
    return *this;
}

Integer Integer::operator>>=(const Integer other) {
    *this = *this >> other;
    return *this;
}

Integer Integer::operator-() const {
    if (is_bignum())
        return -to_bigint();
    return -to_nat_int_t();
}

Integer Integer::operator+(const Integer other) const {
    if (is_bignum() || other.is_bignum()) {
        return to_bigint() + other.to_bigint();
    }

    nat_int_t result;
    auto overflowed = __builtin_add_overflow(to_nat_int_t(), other.to_nat_int_t(), &result);
    if (overflowed)
        return to_bigint() + other.to_bigint();

    return result;
}

Integer Integer::operator-(const Integer other) const {
    if (is_bignum() || other.is_bignum())
        return to_bigint() - other.to_bigint();

    nat_int_t result;
    auto overflowed = __builtin_sub_overflow(to_nat_int_t(), other.to_nat_int_t(), &result);
    if (overflowed)
        return to_bigint() - other.to_bigint();

    return to_nat_int_t() - other.to_nat_int_t();
}

bool Integer::will_multiplication_overflow(nat_int_t a, nat_int_t b) {
    if (a == 0 || b == 0) return false;

    if (a == b)
        return a > NAT_MAX_FIXNUM_SQRT || a < -NAT_MAX_FIXNUM_SQRT;

    auto min_fraction = (NAT_MIN_FIXNUM) / b;
    auto max_fraction = (NAT_MAX_FIXNUM) / b;

    return ((a > 0 && b > 0 && max_fraction <= a)
        || (a > 0 && b < 0 && min_fraction <= a)
        || (a < 0 && b > 0 && min_fraction >= a)
        || (a < 0 && b < 0 && max_fraction >= a));
}

Integer Integer::operator*(const Integer other) const {
    if (is_bignum() || other.is_bignum() || will_multiplication_overflow(to_nat_int_t(), other.to_nat_int_t())) {
        return to_bigint() * other.to_bigint();
    }

    return to_nat_int_t() * other.to_nat_int_t();
}

Integer Integer::operator/(const Integer other) const {
    if (is_bignum() || other.is_bignum())
        return to_bigint() / other.to_bigint();

    auto dividend = to_nat_int_t();
    auto divisor = other.to_nat_int_t();
    auto quotient = dividend / divisor;
    auto remainder = dividend % divisor;

    // Correct division result downwards if up-rounding happened,
    // (for non-zero remainder of sign different than the divisor).
    bool correction = (remainder != 0 && ((remainder < 0) != (divisor < 0)));
    return quotient - correction;
}

Integer Integer::div_c(const Integer other) const {
    if (is_bignum() || other.is_bignum())
        return to_bigint().c_div(other.to_bigint());

    return to_nat_int_t() / other.to_nat_int_t();
}

Integer Integer::operator%(const Integer other) const {
    if (is_bignum() || other.is_bignum())
        return to_bigint() % other.to_bigint();

    auto remainder = to_nat_int_t() % other.to_nat_int_t();
    if (remainder != 0 && signbit(remainder) != signbit(other.to_nat_int_t()))
        remainder += other.to_nat_int_t();

    return remainder;
}

Integer Integer::modulo_c(const Integer other) const {
    if (is_bignum() || other.is_bignum())
        return to_bigint().c_mod(other.to_bigint());

    return to_nat_int_t() % other.to_nat_int_t();
}

double Integer::operator+(const double &other) const {
    return to_double() + other;
}

double Integer::operator-(const double &other) const {
    return to_double() - other;
}

double Integer::operator*(const double &other) const {
    return to_double() * other;
}

double Integer::operator/(const double &other) const {
    return to_double() / other;
}

Integer Integer::operator++() {
    return *this += 1;
}

Integer Integer::operator--() {
    return *this -= 1;
}

bool Integer::operator<(const Integer other) const {
    if (is_bignum()) {
        if (other.is_bignum())
            return *(BigInt *)m_value < *(BigInt *)other.m_value;
        else if (is_negative())
            return true;
        else
            return false;
    } else if (other.is_bignum()) {
        if (other.is_negative())
            return false;
        else
            return true;
    }

    return to_nat_int_t() < other.to_nat_int_t();
}

bool Integer::operator<=(const Integer other) const {
    return *this < other || *this == other;
}

bool Integer::operator>(const Integer other) const {
    return !(*this <= other);
}

bool Integer::operator>=(const Integer other) const {
    return !(*this < other);
}

bool Integer::operator==(const Integer other) const {
    if (is_bignum()) {
        if (other.is_bignum())
            return *(BigInt *)m_value == *(BigInt *)other.m_value;
        else
            return false;
    } else if (other.is_bignum()) {
        return false;
    }

    return to_nat_int_t() == other.to_nat_int_t();
}

bool Integer::operator!=(const Integer other) const {
    return !(*this == other);
}

bool Integer::operator<(const double &other) const {
    if (is_bignum())
        return to_bigint() < other;
    return to_double() < other;
}

bool Integer::operator>(const double &other) const {
    if (is_bignum())
        return to_bigint() > other;
    return to_double() > other;
}

bool Integer::operator<=(const double &other) const {
    if (is_bignum())
        return to_bigint() <= other;
    return to_double() <= other;
}

bool Integer::operator>=(const double &other) const {
    if (is_bignum())
        return to_bigint() >= other;
    return to_double() >= other;
}

bool Integer::operator==(const double &other) const {
    if (is_bignum())
        return to_bigint() == other;

    return to_double() == other;
}

bool Integer::operator!=(const double &other) const {
    if (is_bignum())
        return to_bigint() != other;

    return to_double() != other;
}

Integer Integer::operator<<(const Integer other) const {
    if (other.is_bignum())
        NAT_UNREACHABLE();
    if (is_bignum())
        return to_bigint() << other.to_nat_int_t();

    auto pow_result = pow(2, other.to_nat_int_t());
    if (pow_result.is_bignum())
        return to_bigint() << other.to_nat_int_t();
    else if (will_multiplication_overflow(to_nat_int_t(), pow_result.to_nat_int_t()))
        return to_bigint() << other.to_nat_int_t();

    if (other.to_nat_int_t() < 0)
        return to_nat_int_t() >> ::abs(other.to_nat_int_t());

    if (to_nat_int_t() < 0)
        return static_cast<nat_int_t>(static_cast<uint64_t>(to_nat_int_t()) << other.to_nat_int_t());

    return to_nat_int_t() << other.to_nat_int_t();
}

Integer Integer::operator>>(const Integer other) const {
    if (other.is_bignum())
        NAT_UNREACHABLE();
    if (is_bignum())
        return to_bigint() >> other.to_nat_int_t();

    if (other.to_nat_int_t() >= (nat_int_t)sizeof(nat_int_t) * 8)
        return is_negative() ? -1 : 0;

    return to_nat_int_t() >> other.to_nat_int_t();
}

Integer Integer::operator&(const Integer other) const {
    if (is_bignum() || other.is_bignum())
        return to_bigint() & other.to_bigint();
    return to_nat_int_t() & other.to_nat_int_t();
}

Integer Integer::operator|(const Integer other) const {
    if (is_bignum() || other.is_bignum())
        return to_bigint() | other.to_bigint();
    return to_nat_int_t() | other.to_nat_int_t();
}

Integer Integer::operator^(const Integer other) const {
    if (is_bignum() || other.is_bignum())
        return to_bigint() ^ other.to_bigint();
    return to_nat_int_t() ^ other.to_nat_int_t();
}

Integer Integer::operator~() const {
    if (is_bignum())
        return ~to_bigint();
    return ~to_nat_int_t();
};

bool Integer::is_negative() const {
    if (is_bignum())
        return to_bigint().is_negative();
    return to_nat_int_t() < 0;
}

double Integer::to_double() const {
    if (is_bignum())
        return to_bigint().to_double();
    else
        return static_cast<double>(to_nat_int_t());
}

TM::String Integer::to_string() const {
    if (is_bignum()) {
        return to_bigint().to_string();
    }
    return TM::String(to_nat_int_t());
}

Integer Integer::bit_length() const {
    // If it is equal to NAT_MAX_FIXNUM we cannot increment it later, instead we have to use the
    // bignum branch.
    if (is_bignum() || to_nat_int_t() == NAT_MAX_FIXNUM) {
        auto binary = to_bigint().to_binary();
        for (size_t i = 0; i < binary.length(); ++i)
            if (binary[i] == (to_bigint().is_negative() ? '0' : '1'))
                return (nat_int_t)(binary.length() - i);
    }

    auto nat_int = to_nat_int_t();
    return ceil(log2(nat_int < 0 ? -nat_int : nat_int + 1));
}

Integer operator+(const long long &lhs, const Integer rhs) {
    return Integer(lhs) + rhs;
}

Integer operator-(const long long &lhs, const Integer rhs) {
    return Integer(lhs) - rhs;
}

Integer operator*(const long long &lhs, const Integer rhs) {
    return Integer(lhs) * rhs;
}

Integer operator/(const long long &lhs, const Integer rhs) {
    return Integer(lhs) / rhs;
}

Integer operator<(const long long &lhs, const Integer rhs) {
    return Integer(lhs) < rhs;
}

Integer operator>(const long long &lhs, const Integer rhs) {
    return Integer(lhs) > rhs;
}

Integer operator<=(const long long &lhs, const Integer rhs) {
    return Integer(lhs) <= rhs;
}

Integer operator>=(const long long &lhs, const Integer rhs) {
    return Integer(lhs) >= rhs;
}

Integer operator==(const long long &lhs, const Integer rhs) {
    return Integer(lhs) == rhs;
}

Integer operator!=(const long long &lhs, const Integer rhs) {
    return Integer(lhs) != rhs;
}

Integer pow(Integer lhs, Integer rhs) {
    if (rhs == 0) return 1;
    if (rhs == 1) return lhs;
    if (lhs == 0) return 0;
    if (lhs == 1) return 1;
    if (lhs == -1) return rhs % 2 ? 1 : -1;

    Integer result = 1;
    bool negative = lhs < 0;
    if (negative) lhs = -lhs;
    negative = negative && (rhs % 2).to_nat_int_t();

    while (rhs != 0) {
        while (rhs % 2 == 0) {
            rhs >>= 1;
            lhs *= lhs;
        }

        result *= lhs;
        --rhs;
    }

    if (negative)
        return -result;
    return result;
}

Integer abs(const Integer other) {
    if (other.is_negative())
        return -other;
    return other;
}

Integer gcd(const Integer dividend, const Integer divisor) {
    if (dividend.is_bignum() || divisor.is_bignum())
        return BigInt::gcd(dividend.to_bigint(), divisor.to_bigint());

    auto result = abs(dividend);
    auto divisor_abs = abs(divisor);
    auto remainder = divisor_abs;

    while (remainder != 0) {
        remainder = result % divisor_abs;
        result = divisor_abs;
        divisor_abs = remainder;
    }

    return result;
}

Integer sqrt(const Integer other) {
    if (other.is_bignum())
        return other.to_bigint().sqrt();
    return static_cast<nat_int_t>(::sqrt(other.to_nat_int_t()));
}

}
