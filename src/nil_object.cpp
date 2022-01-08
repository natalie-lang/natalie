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

Value NilObject::to_s(Env *env) {
    return new StringObject { "" };
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

Value NilObject::inspect(Env *env) {
    return new StringObject { "nil" };
}

}
