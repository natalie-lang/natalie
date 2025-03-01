#include "natalie.hpp"

namespace Natalie {

bool FalseObject::and_method(const Env *env, const Value, const Value) {
    return false;
}

bool FalseObject::or_method(const Env *env, const Value, const Value other) {
    return other.is_truthy();
}

Value FalseObject::to_s(const Env *env, const Value) {
    if (!s_string)
        s_string = new StringObject { "false" };
    return s_string;
}

void FalseObject::visit_children(Visitor &visitor) const {
    Object::visit_children(visitor);
    if (s_string)
        visitor.visit(s_string);
}

}
