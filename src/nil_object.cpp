#include "natalie.hpp"

namespace Natalie {

bool NilObject::and_method(Env *env, Value) {
    return false;
}

bool NilObject::or_method(Env *env, Value other) {
    return other->is_truthy();
}

Value NilObject::eqtilde(Env *env, Value) {
    return NilObject::the();
}

Value NilObject::rationalize(Env *env, Value) {
    return this->to_r(env);
}

Value NilObject::to_s(Env *env) {
    if (!s_string)
        s_string = new StringObject { "" };
    return s_string;
}

Value NilObject::to_a(Env *env) {
    return new ArrayObject {};
}

Value NilObject::to_h(Env *env) {
    return new HashObject {};
}

Value NilObject::to_f(Env *env) {
    return new FloatObject { 0.0 };
}

Value NilObject::to_i(Env *env) {
    return Value::integer(0);
}

Value NilObject::to_r(Env *env) {
    return new RationalObject { new IntegerObject { 0 }, new IntegerObject { 1 } };
}

Value NilObject::inspect(Env *env) {
    return new StringObject { "nil" };
}

void NilObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    if (s_string)
        visitor.visit(s_string);
}

}
