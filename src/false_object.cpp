#include "natalie.hpp"

namespace Natalie {

ValuePtr FalseObject::to_s(Env *env) {
    return new StringObject { "false" };
}

}
