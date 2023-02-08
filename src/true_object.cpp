#include "natalie.hpp"

namespace Natalie {

bool TrueObject::and_method(const Env *env, Value other) const {
    return other->is_truthy();
}

bool TrueObject::or_method(const Env *env, const Value other) const {
    return true;
}

bool TrueObject::xor_method(const Env *env, Value other) const {
    return other->is_falsey();
}

Value TrueObject::to_s(const Env *env) const {
    if (!s_string)
        s_string = new StringObject { "true" };
    return s_string;
}

void TrueObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    if (s_string)
        visitor.visit(s_string);
}

}
