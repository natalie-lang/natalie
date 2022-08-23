#pragma once

#include "tm/string.hpp"

namespace Natalie {
class StringUptoIterator {
public:
    StringUptoIterator(const TM::String string)
        : m_string(string) {
        if (string.contains_only_digits())
            m_treat_like_integer = true;
    }

    TM::String next();
    TM::String peek() const;

private:
    TM::String m_string;
    bool m_treat_like_integer { false };
};
}
