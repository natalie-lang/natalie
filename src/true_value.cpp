#include "natalie.hpp"

namespace Natalie {

Value *TrueValue::to_s(Env *env) {
    return new StringValue { env, "true" };
}

}
