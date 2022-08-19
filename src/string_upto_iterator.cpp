#include "natalie/string_upto_iterator.hpp"
#include "natalie/integer.hpp"

namespace Natalie {
const TM::String StringUptoIterator::next() {
    return m_string = peek();
}

const TM::String StringUptoIterator::peek() const {
    if (m_treat_like_integer)
        return TM::String((Integer(m_string) + 1).to_nat_int_t());

    if (m_string == "9")
        return ":";
    else if (m_string == "Z")
        return "[";
    else if (m_string == "z")
        return "{";

    return m_string.successive();
}
}
