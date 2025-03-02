#include "natalie.hpp"

namespace Natalie {

bool TrueMethods::and_method(const Env *env, const Value, const Value other) {
    return other.is_truthy();
}

bool TrueMethods::or_method(const Env *env, const Value, const Value) {
    return true;
}

bool TrueMethods::xor_method(const Env *env, const Value, const Value other) {
    return other.is_falsey();
}

Value TrueMethods::to_s(const Env *env, const Value) {
    if (!s_string)
        s_string = new StringObject { "true" };
    return s_string;
}

}
