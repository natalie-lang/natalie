#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ExceptionValue : Value {
    ExceptionValue(Env *env)
        : Value { Value::Type::Exception, NAT_OBJECT->const_get(env, "Exception", true)->as_class() } { }

    ExceptionValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Exception, klass } { }

    ExceptionValue(Env *env, ClassValue *klass, const char *message)
        : Value { Value::Type::Exception, klass }
        , m_message { message } {
        assert(m_message);
    }

    const char *message() { return m_message; }
    void set_message(const char *message) { m_message = strdup(message); }

    ArrayValue *backtrace() { return m_backtrace; }
    void build_backtrace(Env *);

private:
    const char *m_message { nullptr };
    ArrayValue *m_backtrace { nullptr };
};

}
