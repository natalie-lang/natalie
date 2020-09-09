#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct IoValue : Value {
    IoValue(Env *env)
        : Value { Value::Type::Io, env->Object()->const_fetch("IO")->as_class() } { }

    IoValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Io, klass } { }

    IoValue(Env *env, int fileno)
        : Value { Value::Type::Io, env->Object()->const_fetch("IO")->as_class() }
        , m_fileno { fileno } { }

    int fileno() { return m_fileno; }
    void set_fileno(int fileno) { m_fileno = fileno; }

    Value *initialize(Env *, Value *);
    Value *read(Env *, Value *);
    Value *write(Env *, ssize_t, Value **);
    Value *puts(Env *, ssize_t, Value **);
    Value *print(Env *, ssize_t, Value **);
    Value *close(Env *);
    Value *seek(Env *, Value *, Value *);

private:
    int m_fileno { 0 };
};

}
