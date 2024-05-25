#include "natalie/bigint.hpp"

namespace Natalie {

BigInt &BigInt::operator>>=(unsigned long long shift) {
    unsigned long long bits = bigint_bitlength(data);
    if (shift > bits) {
        if (*this < 0ll)
            *this = -1;
        else
            *this = 0;
        return *this;
    }

    bigint_shift_right(data, data, shift);
    return *this;
}

double BigInt::to_double() const {
    if (*this < std::numeric_limits<double>::lowest()) {
        return -std::numeric_limits<double>::infinity();
    } else if (*this > DBL_MAX) {
        return std::numeric_limits<double>::infinity();
    } else {
        return strtod(this->to_string().c_str(), NULL);
    }
}

TM::String BigInt::to_string(int base) const {
    const auto size = bigint_write_size(data, base);
    // includes a space for possible `-` and one for the null terminator `\0`
    char buf[size];
    int _size = size;
    bigint_write_base(buf, &_size, data, base, 1);
    return TM::String(buf);
}

BigInt &BigInt::operator/=(const BigInt &b) {
    BigInt remainder;
    bigint_div_mod(data, remainder.data, data, b.data);

    // Correct division result downwards if up-rounding happened,
    // (for non-zero remainder of sign different than the divisor).
    bool corr = (remainder != 0ll && ((remainder < 0ll) != (b < 0ll)));
    bigint_sub_word(data, data, corr ? 1 : 0);

    return *this;
}

BigInt &BigInt::operator%=(const BigInt &b) {
    bigint_mod(data, data, b.data);

    if (*this != 0ll && b.data->neg != data->neg)
        bigint_add(data, data, b.data);

    return *this;
}
}
