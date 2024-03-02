#pragma once

/*
 * Experimental file to emulate the MRI API in Natalie.
 */

#include "natalie.hpp"

using VALUE = Value;

inline size_t RSTRING_LEN(Value string) {
    return string->as_string()->bytesize();
}
