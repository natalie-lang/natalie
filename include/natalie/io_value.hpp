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

class IoValue : public Value {
public:
    IoValue()
        : Value { Value::Type::Io, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("IO"))->as_class() } { }

    IoValue(ClassValue *klass)
        : Value { Value::Type::Io, klass } { }

    IoValue(int fileno)
        : Value { Value::Type::Io, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("IO"))->as_class() }
        , m_fileno { fileno } { }

    virtual ~IoValue() override {
        if (m_fileno == STDIN_FILENO || m_fileno == STDOUT_FILENO || m_fileno == STDERR_FILENO)
            return;
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
    bool is_closed() { return m_closed; }

private:
    int m_fileno { -1 };
    bool m_closed { false };
};

}
