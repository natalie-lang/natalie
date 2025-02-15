#include "natalie.hpp"

namespace Natalie {

bool NilObject::and_method(const Value, const Value) {
    return false;
}

bool NilObject::or_method(const Value, Value other) {
    return other.is_truthy();
}

Value NilObject::eqtilde(const Value, const Value) {
    return NilObject::the();
}

Value NilObject::rationalize(const Value self, const Value) {
    return to_r(self);
}

Value NilObject::to_s(const Value) {
    if (!s_string)
        s_string = new StringObject { "" };
    return s_string;
}

Value NilObject::to_a(const Value) {
    return new ArrayObject {};
}

Value NilObject::to_c(const Value) {
    return new ComplexObject { Value::integer(0) };
}

Value NilObject::to_h(const Value) {
    return new HashObject {};
}

Value NilObject::to_f(const Value) {
    return new FloatObject { 0.0 };
}

Value NilObject::to_i(const Value) {
    return Value::integer(0);
}

Value NilObject::to_r(const Value) {
    return new RationalObject { Value::integer(0), Value::integer(1) };
}

Value NilObject::inspect(const Value) {
    return new StringObject { "nil" };
}

void NilObject::visit_children(Visitor &visitor) const {
    Object::visit_children(visitor);
    if (s_string)
        visitor.visit(s_string);
}

}
