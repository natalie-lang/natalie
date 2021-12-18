#include "natalie.hpp"

namespace Natalie {

Value FalseObject::to_s(Env *env) {
    return new StringObject { "false" };
}

}
