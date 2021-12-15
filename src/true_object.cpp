#include "natalie.hpp"

namespace Natalie {

ValuePtr TrueObject::to_s(Env *env) {
    return new StringObject { "true" };
}

}
