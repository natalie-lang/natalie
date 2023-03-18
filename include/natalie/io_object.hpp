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

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_external_encoding);
        visitor.visit(m_internal_encoding);
    }

    static Value size_fn(Env *env, Value self, Args, Block *) {
        return Value(NilObject::the());
    }

    static Value read_file(Env *, Value);
    static Value write_file(Env *, Value, Value);

    int fileno() const { return m_fileno; }
    void set_fileno(int fileno) { m_fileno = fileno; }

    Value append(Env *, Value);
    Value initialize(Env *, Value);
    Value read(Env *, Value) const;
    Value write(Env *, Args) const;
    Value gets(Env *) const;
    Value puts(Env *, Args) const;
    Value print(Env *, Args) const;
    Value close(Env *);
    Value seek(Env *, Value, Value) const;
    Value stat(Env *) const;
    bool is_closed() const { return m_closed; }

    void set_external_encoding(Env *env, EncodingObject *enc) {
        m_external_encoding = enc;
    }
    void set_internal_encoding(Env *env, EncodingObject *enc) {
        m_internal_encoding = enc;
    }

protected:
    int write(Env *, Value) const;

private:
    EncodingObject *m_external_encoding { nullptr };
    EncodingObject *m_internal_encoding { nullptr };
    int m_fileno { -1 };
    bool m_closed { false };
};

}
