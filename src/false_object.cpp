#include "natalie.hpp"

namespace Natalie {

bool FalseObject::and_method(Env *env, Value other) {
    return false;
}

bool FalseObject::or_method(Env *env, Value other) {
    return other->is_truthy();
}

Value FalseObject::to_s(Env *env) {
    return new StringObject { "false" };
}

}
