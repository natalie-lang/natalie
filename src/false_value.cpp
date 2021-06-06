#include "natalie.hpp"

namespace Natalie {

ValuePtr FalseValue::to_s(Env *env) {
    return new StringValue { "false" };
}

}
