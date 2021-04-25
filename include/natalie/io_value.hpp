#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

#ifdef fileno
#undef fileno
#endif

namespace Natalie {

struct IoValue : Value {
    IoValue(Env *env)
        : Value { Value::Type::Io, env->Object()->const_fetch(env, SymbolValue::intern(env, "IO"))->as_class() } { }

    IoValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Io, klass } { }

    IoValue(Env *env, int fileno)
        : Value { Value::Type::Io, env->Object()->const_fetch(env, SymbolValue::intern(env, "IO"))->as_class() }
        , m_fileno { fileno } { }

    virtual ~IoValue() {
        if (!m_closed && m_fileno != -1) {
            ::close(m_fileno);
            m_closed = true;
        }
    }

    static ValuePtr read_file(Env *, ValuePtr);

    int fileno() { return m_fileno; }
    void set_fileno(int fileno) { m_fileno = fileno; }

    ValuePtr initialize(Env *, ValuePtr);
    ValuePtr read(Env *, ValuePtr);
    ValuePtr write(Env *, size_t, ValuePtr *);
    ValuePtr puts(Env *, size_t, ValuePtr *);
    ValuePtr print(Env *, size_t, ValuePtr *);
    ValuePtr close(Env *);
    ValuePtr seek(Env *, ValuePtr, ValuePtr);

private:
    int m_fileno { -1 };
    bool m_closed { false };
};

}
