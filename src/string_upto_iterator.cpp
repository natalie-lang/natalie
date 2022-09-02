#include "natalie/string_upto_iterator.hpp"
#include "natalie/integer.hpp"

namespace Natalie {
TM::Optional<TM::String> StringUptoIterator::next() {
    auto peeked = peek();
    if (peeked.present())
        m_current = peeked.value();
    return peeked;
}

TM::Optional<TM::String> StringUptoIterator::peek() const {
    if (to_reached())
        return {};

    TM::String result;
    if (m_current.present()) {
        auto current = m_current.value();
        if (m_treat_as_integer) {
            result = TM::String((Integer(current) + 1).to_nat_int_t());
        } else {
            // special handling for single characters
            if (current == "9")
                result = ":";
            else if (current == "Z")
                result = "[";
            else if (current == "z")
                result = "{";
            else
                result = current.successive();
        }
    } else {
        result = m_from;
    }

    if (m_has_to && m_exclusive && result == m_to)
        return {};

    return result;
}

bool StringUptoIterator::to_reached() const {
    if (!m_has_to)
        return false;

    if (!m_current.present())
        return false;

    if (m_treat_as_integer) {
        return Integer(m_current.value()) >= Integer(m_to);
    } else {
        return m_current.value() >= m_to;
    }
}
}
