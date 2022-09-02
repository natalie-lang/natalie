#pragma once

#include "tm/optional.hpp"
#include "tm/string.hpp"

namespace Natalie {
class StringUptoIterator {
public:
    StringUptoIterator(const TM::String string)
        : m_from(string) {
        if (string.contains_only_digits())
            m_treat_as_integer = true;
    }

    StringUptoIterator(const TM::String from, const TM::String to, bool exclusive = false)
        : m_from(from)
        , m_to(to)
        , m_has_to(true)
        , m_exclusive(exclusive) {
        if (from.contains_only_digits() && to.contains_only_digits())
            m_treat_as_integer = true;
    }

    TM::Optional<TM::String> next();
    TM::Optional<TM::String> peek() const;
    bool treat_as_integer() const { return m_treat_as_integer; }

private:
    bool to_reached() const;

    TM::String m_from;
    TM::Optional<TM::String> m_current {};
    TM::String m_to;
    bool m_has_to { false };
    bool m_treat_as_integer { false };
    bool m_exclusive { false };
};
}
