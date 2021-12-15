#include "natalie.hpp"

namespace Natalie {

Value NilObject::eqtilde(Env *env, Value) {
    return NilObject::the();
}

Value NilObject::to_s(Env *env) {
    return new StringObject { "" };
}

Value NilObject::to_a(Env *env) {
    return new ArrayObject {};
}

Value NilObject::to_i(Env *env) {
    return Value::integer(0);
}

Value NilObject::inspect(Env *env) {
    return new StringObject { "nil" };
}

}
