#include "natalie.hpp"

namespace Natalie {

bool FalseObject::and_method(Env *env, Value other) {
    return false;
}

bool FalseObject::or_method(Env *env, Value other) {
    return other->is_truthy();
}

Value FalseObject::to_s(Env *env) {
    if (!s_string)
        s_string = new StringObject { "false" };
    return s_string;
}

void FalseObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    if (s_string)
        visitor.visit(s_string);
}

}
