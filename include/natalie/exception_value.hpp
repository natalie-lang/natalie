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
        : Value { Value::Type::Exception, env->Object()->const_fetch("Exception")->as_class() } { }

    ExceptionValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Exception, klass } { }

    ExceptionValue(Env *env, ClassValue *klass, const char *message)
        : Value { Value::Type::Exception, klass }
        , m_message { message } {
        assert(m_message);
    }

    const char *message() { return m_message; }
    void set_message(const char *message) { m_message = GC_STRDUP(message); }

    Value *initialize(Env *, Value *);
    Value *inspect(Env *);
    Value *message(Env *);

    const ArrayValue *backtrace() { return m_backtrace; }
    Value *backtrace(Env *);
    void build_backtrace(Env *);

private:
    const char *m_message { nullptr };
    ArrayValue *m_backtrace { nullptr };
};

}
