#pragma once

#include "natalie/forward.hpp"

namespace Natalie {

class MarshalModule {
public:
    static Value float_to_string(Env *, Value);
    static void set_exception_cause(Env *, Value, Value);
};

}
