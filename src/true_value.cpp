#include "natalie.hpp"

namespace Natalie {

ValuePtr TrueValue::to_s(Env *env) {
    return new StringValue { env, "true" };
}

}
