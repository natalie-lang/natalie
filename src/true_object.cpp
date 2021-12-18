#include "natalie.hpp"

namespace Natalie {

Value TrueObject::to_s(Env *env) {
    return new StringObject { "true" };
}

}
