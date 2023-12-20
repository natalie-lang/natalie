#include "natalie.hpp"

namespace Natalie::Thread {

Value ConditionVariableObject::signal(Env *env) {
    return NilObject::the();
}

Value ConditionVariableObject::wait(Env *env, Value mutex, Value timeout) {
    return NilObject::the();
}

}
