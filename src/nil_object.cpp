#include "natalie.hpp"

namespace Natalie {

ValuePtr NilObject::eqtilde(Env *env, ValuePtr) {
    return NilObject::the();
}

ValuePtr NilObject::to_s(Env *env) {
    return new StringObject { "" };
}

ValuePtr NilObject::to_a(Env *env) {
    return new ArrayObject {};
}

ValuePtr NilObject::to_i(Env *env) {
    return ValuePtr::integer(0);
}

ValuePtr NilObject::inspect(Env *env) {
    return new StringObject { "nil" };
}

}
