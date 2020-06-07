#include "natalie.hpp"

namespace Natalie {

ExceptionValue::ExceptionValue(Env *env, ClassValue *klass, const char *message)
    : ExceptionValue { env, klass } {
    assert(message);
    Value *message_obj = string(env, message);
    // FIXME: this is the only constructor that calls initialize()
    // That's weird and inconsistent.
    this->initialize(env, 1, &message_obj, nullptr);
}

}
