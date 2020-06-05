#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ExceptionValue : Value {
    using Value::Value;

    ExceptionValue(Env *env)
        : Value { env, Value::Type::Exception, const_get(env, NAT_OBJECT, "Exception", true)->as_class() } { }

    ExceptionValue(Env *env, ClassValue *klass)
        : Value { env, Value::Type::Exception, klass } { }

    ExceptionValue(Env *, ClassValue *, const char *);

    char *message { nullptr };
    ArrayValue *backtrace { nullptr };
};

}
