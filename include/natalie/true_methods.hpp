#pragma once

#include <assert.h>

#include "natalie/object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

class TrueMethods : public Object {
public:
    static bool and_method(const Env *, Value, Value);
    static bool or_method(const Env *, Value, Value);
    static bool xor_method(const Env *, Value, Value);
    static Value to_s(const Env *, Value);

    static void visit_string(Cell::Visitor &visitor) {
        visitor.visit(s_string);
    }

private:
    inline static StringObject *s_string = nullptr;
};

}
