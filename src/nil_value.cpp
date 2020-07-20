#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

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
