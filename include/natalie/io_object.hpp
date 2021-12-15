#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

#ifdef fileno
#undef fileno
#endif

namespace Natalie {

class IoObject : public Object {
public:
    IoObject()
        : Object { Object::Type::Io, GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("IO"))->as_class() } { }

    IoObject(ClassObject *klass)
        : Object { Object::Type::Io, klass } { }

    IoObject(int fileno)
        : Object { Object::Type::Io, GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("IO"))->as_class() }
        , m_fileno { fileno } { }

    virtual ~IoObject() override {
        if (m_fileno == STDIN_FILENO || m_fileno == STDOUT_FILENO || m_fileno == STDERR_FILENO)
            return;
        if (!m_closed && m_fileno != -1) {
            ::close(m_fileno);
            m_closed = true;
        }
    }

    static ValuePtr read_file(Env *, ValuePtr);

    int fileno() const { return m_fileno; }
    void set_fileno(int fileno) { m_fileno = fileno; }

    ValuePtr initialize(Env *, ValuePtr);
    ValuePtr read(Env *, ValuePtr) const;
    ValuePtr write(Env *, size_t, ValuePtr *) const;
    ValuePtr puts(Env *, size_t, ValuePtr *) const;
    ValuePtr print(Env *, size_t, ValuePtr *) const;
    ValuePtr close(Env *);
    ValuePtr seek(Env *, ValuePtr, ValuePtr) const;
    bool is_closed() const { return m_closed; }

private:
    int m_fileno { -1 };
    bool m_closed { false };
};

}
