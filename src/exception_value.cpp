#include "natalie.hpp"

namespace Natalie {

ExceptionValue::ExceptionValue(Env *env, ClassValue *klass, const char *message)
    : ExceptionValue { env, klass } {
    assert(message);
    Value *message_obj = string(env, message);
    initialize(env, this, 1, &message_obj, NULL);
}

}
