#include "natalie.hpp"

namespace Natalie {

bool FalseObject::and_method(const Env *env, const Value other) const {
    return false;
}

bool FalseObject::or_method(const Env *env, Value other) const {
    return other->is_truthy();
}

Value FalseObject::to_s(const Env *env) const {
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
