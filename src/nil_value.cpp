#include "natalie.hpp"

namespace Natalie {

ValuePtr NilValue::eqtilde(Env *env, ValuePtr ) {
    return env->nil_obj();
}

ValuePtr NilValue::to_s(Env *env) {
    return new StringValue { env, "" };
}

ValuePtr NilValue::to_a(Env *env) {
    return new ArrayValue { env };
}

ValuePtr NilValue::to_i(Env *env) {
    return new IntegerValue { env, 0 };
}

ValuePtr NilValue::inspect(Env *env) {
    return new StringValue { env, "nil" };
}

}
