#include "natalie.hpp"

namespace Natalie {

Value *NilValue::eqtilde(Env *env, Value *) {
    return env->nil_obj();
}

Value *NilValue::to_s(Env *env) {
    return new StringValue { env, "" };
}

Value *NilValue::to_a(Env *env) {
    return new ArrayValue { env };
}

Value *NilValue::to_i(Env *env) {
    return new IntegerValue { env, 0 };
}

Value *NilValue::inspect(Env *env) {
    return new StringValue { env, "nil" };
}

}
