#pragma once

#include "natalie/forward.hpp"

namespace Natalie {

class NumberParser {
public:
    static FloatObject *string_to_f(const StringObject *str);
};

}
