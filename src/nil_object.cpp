#include "natalie.hpp"

namespace Natalie {

bool NilObject::and_method(const Env *env, const Value) const {
    return false;
}

bool NilObject::or_method(const Env *env, Value other) const {
    return other->is_truthy();
}

Value NilObject::eqtilde(const Env *env, const Value) const {
    return NilObject::the();
}

Value NilObject::rationalize(const Env *env, const Value) const {
    return this->to_r(env);
}

Value NilObject::to_s(const Env *env) const {
    if (!s_string)
        s_string = new StringObject { "" };
    return s_string;
}

Value NilObject::to_a(const Env *env) const {
    return new ArrayObject {};
}

Value NilObject::to_c(const Env *env) const {
    return new ComplexObject { new IntegerObject { 0 } };
}

Value NilObject::to_h(const Env *env) const {
    return new HashObject {};
}

Value NilObject::to_f(const Env *env) const {
    return new FloatObject { 0.0 };
}

Value NilObject::to_i(const Env *env) const {
    return Value::integer(0);
}

Value NilObject::to_r(const Env *env) const {
    return new RationalObject { new IntegerObject { 0 }, new IntegerObject { 1 } };
}

Value NilObject::inspect(const Env *env) const {
    return new StringObject { "nil" };
}

void NilObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    if (s_string)
        visitor.visit(s_string);
}

}
