#include "natalie.hpp"

namespace Natalie {

Value *FalseValue::to_s(Env *env) {
    return new StringValue { env, "false" };
}

}
