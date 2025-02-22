#pragma once

#include "natalie/forward.hpp"
#include "tm/string.hpp"

namespace Natalie {

class NumberParser {
public:
    NumberParser(const StringObject *str);

    static FloatObject *string_to_f(const StringObject *str);

private:
    const TM::String m_str;
};

}
