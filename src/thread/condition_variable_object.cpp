#include "natalie.hpp"

namespace Natalie::Thread {

Value ConditionVariableObject::signal(Env *env) {
    // https://en.cppreference.com/w/cpp/thread/condition_variable
    return NilObject::the();
}

Value ConditionVariableObject::wait(Env *env, Value mutex, Value timeout) {
    // https://en.cppreference.com/w/cpp/thread/condition_variable
    return NilObject::the();
}

}
