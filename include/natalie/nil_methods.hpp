#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class NilMethods {
public:
    static bool and_method(Value, Value);
    static bool or_method(Value, Value);
    static Value eqtilde(Value, Value);
    static Value rationalize(Value, Optional<Value>);
    static Value to_s(Value);
    static Value to_a(Value);
    static Value to_c(Value);
    static Value to_h(Value);
    static Value to_f(Value);
    static Value to_i(Value);
    static Value to_r(Value);
    static Value inspect(Value);

    static void visit_string(Cell::Visitor &visitor) {
        visitor.visit(s_string);
    }

private:
    inline static StringObject *s_string = nullptr;
};

}
