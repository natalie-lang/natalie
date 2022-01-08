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
        : Object { Object::Type::Io, GlobalEnv::the()->Object()->const_fetch("IO"_s)->as_class() } { }

    IoObject(ClassObject *klass)
        : Object { Object::Type::Io, klass } { }

    IoObject(int fileno)
        : Object { Object::Type::Io, GlobalEnv::the()->Object()->const_fetch("IO"_s)->as_class() }
        , m_fileno { fileno } { }

    virtual ~IoObject() override {
        if (m_fileno == STDIN_FILENO || m_fileno == STDOUT_FILENO || m_fileno == STDERR_FILENO)
            return;
        if (!m_closed && m_fileno != -1) {
            ::close(m_fileno);
            m_closed = true;
        }
    }

    static Value read_file(Env *, Value);
    static Value write_file(Env *, Value, Value);

    int fileno() const { return m_fileno; }
    void set_fileno(int fileno) { m_fileno = fileno; }

    Value initialize(Env *, Value);
    Value read(Env *, Value) const;
    Value write(Env *, size_t, Value *) const;
    Value puts(Env *, size_t, Value *) const;
    Value print(Env *, size_t, Value *) const;
    Value close(Env *);
    Value seek(Env *, Value, Value) const;
    bool is_closed() const { return m_closed; }

private:
    int m_fileno { -1 };
    bool m_closed { false };
};

}
