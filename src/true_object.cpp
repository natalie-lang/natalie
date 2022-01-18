#include "natalie.hpp"

namespace Natalie {

bool TrueObject::and_method(Env *env, Value other) {
    return other->is_truthy();
}

bool TrueObject::or_method(Env *env, Value other) {
    return true;
}

bool TrueObject::xor_method(Env *env, Value other) {
    return other->is_falsey();
}

Value TrueObject::to_s(Env *env) {
    if (s_string) {
        return s_string;
    }
    s_string = new StringObject { "true" };
    return s_string;
}

}
