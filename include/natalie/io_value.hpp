#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/finalizer.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

#ifdef fileno
#undef fileno
#endif

namespace Natalie {

struct IoValue : Value, finalizer {
    IoValue(Env *env)
        : Value { Value::Type::Io, env->Object()->const_fetch("IO")->as_class() } { }

    IoValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Io, klass } { }

    IoValue(Env *env, int fileno)
        : Value { Value::Type::Io, env->Object()->const_fetch("IO")->as_class() }
        , m_fileno { fileno } { }

    virtual ~IoValue() {
        ::close(m_fileno);
    }

    static Value *read_file(Env *, Value *);

    int fileno() { return m_fileno; }
    void set_fileno(int fileno) { m_fileno = fileno; }

    Value *initialize(Env *, Value *);
    Value *read(Env *, Value *);
    Value *write(Env *, size_t, Value **);
    Value *puts(Env *, size_t, Value **);
    Value *print(Env *, size_t, Value **);
    Value *close(Env *);
    Value *seek(Env *, Value *, Value *);

private:
    int m_fileno { 0 };
};

}
