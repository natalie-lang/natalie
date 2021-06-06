#include "natalie.hpp"

namespace Natalie {

ValuePtr NilValue::eqtilde(Env *env, ValuePtr) {
    return NilValue::the();
}

ValuePtr NilValue::to_s(Env *env) {
    return new StringValue { "" };
}

ValuePtr NilValue::to_a(Env *env) {
    return new ArrayValue {};
}

ValuePtr NilValue::to_i(Env *env) {
    return ValuePtr::integer(0);
}

ValuePtr NilValue::inspect(Env *env) {
    return new StringValue { "nil" };
}

}
