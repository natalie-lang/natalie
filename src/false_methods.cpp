#include "natalie.hpp"

namespace Natalie {

bool FalseMethods::and_method(const Env *env, const Value, const Value) {
    return false;
}

bool FalseMethods::or_method(const Env *env, const Value, const Value other) {
    return other.is_truthy();
}

Value FalseMethods::to_s(const Env *env, const Value) {
    if (!s_string)
        s_string = new StringObject { "false" };
    return s_string;
}

}
