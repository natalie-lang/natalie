#include "natalie.hpp"

namespace Natalie {

Value MutexObject::lock(Env *) {
    return this;
}

Value MutexObject::unlock(Env *) {
    return this;
}

}
