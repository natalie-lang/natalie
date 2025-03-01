#include "natalie.hpp"

namespace Natalie {

bool TrueObject::and_method(const Env *env, const Value, const Value other) {
    return other.is_truthy();
}

bool TrueObject::or_method(const Env *env, const Value, const Value) {
    return true;
}

bool TrueObject::xor_method(const Env *env, const Value, const Value other) {
    return other.is_falsey();
}

Value TrueObject::to_s(const Env *env, const Value) {
    if (!s_string)
        s_string = new StringObject { "true" };
    return s_string;
}

void TrueObject::visit_children(Visitor &visitor) const {
    Object::visit_children(visitor);
    if (s_string)
        visitor.visit(s_string);
}

}
