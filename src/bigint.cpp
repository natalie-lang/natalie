#include "natalie/bigint.hpp"

namespace Natalie {

BigInt &BigInt::operator>>=(unsigned long long shift) {
    unsigned long long bits = bigint_bitlength(m_data);
    if (shift > bits) {
        if (m_data->neg)
            *this = -1;
        else
            *this = 0;
        return *this;
    }

    if (m_data->neg) {
        bigint_convert_to_twos_complement(m_data, m_data->size + 1);
        bigint_shift_right(m_data, m_data, shift);
        bigint_twos_complement_sign_extend(m_data);
        bigint_convert_from_twos_complement(m_data);
        return *this;
    }

    bigint_shift_right(m_data, m_data, shift);
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
    const auto size = bigint_write_size(m_data, base);
    // includes a space for possible `-` and one for the null terminator `\0`
    char buf[size];
    int _size = size;
    bigint_write_base(buf, &_size, m_data, base, 1);
    return TM::String(buf);
}

TM::String BigInt::to_binary() const {
    if (m_data->neg) {
        BigInt temp;
        bigint_cpy(temp.m_data, m_data);
        bigint_convert_to_twos_complement(temp.m_data, m_data->size + 1);
        return temp.to_binary();
    }

    return to_string(2);
}

BigInt &BigInt::operator/=(const BigInt &b) {
    BigInt remainder;
    bigint_div_mod(m_data, remainder.m_data, m_data, b.m_data);

    // Correct division result downwards if up-rounding happened,
    // (for non-zero remainder of sign different than the divisor).
    bool corr = (remainder != 0ll && ((remainder < 0ll) != (b < 0ll)));
    bigint_sub_word(m_data, m_data, corr ? 1 : 0);

    return *this;
}

BigInt &BigInt::operator%=(const BigInt &b) {
    bigint_mod(m_data, m_data, b.m_data);

    if (*this != 0ll && b.m_data->neg != m_data->neg)
        bigint_add(m_data, m_data, b.m_data);

    return *this;
}
}
