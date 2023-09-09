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
        visitor.visit(m_path);
    }

    static Value size_fn(Env *env, Value self, Args, Block *) {
        return Value(NilObject::the());
    }

    Value advise(Env *, Value, Value, Value);
    Value append(Env *, Value);
    Value close(Env *);
    Value each_byte(Env *, Block *);
    Value external_encoding() const { return m_external_encoding; }
    Value fcntl(Env *, Value, Value = nullptr);
    int fdatasync(Env *);
    int fileno() const;
    int fileno(Env *) const;
    int fsync(Env *);
    Value getbyte(Env *);
    Value gets(Env *) const;
    Value initialize(Env *, Value, Value = nullptr);
    Value inspect() const;
    Value internal_encoding() const { return m_internal_encoding; }
    bool is_closed() const { return m_closed; }
    bool is_eof(Env *);
    bool isatty(Env *) const;
    int pos(Env *);
    Value puts(Env *, Args);
    void puts(Env *, Value);
    void putstr(Env *, StringObject *);
    void putary(Env *, ArrayObject *);
    Value print(Env *, Args) const;
    Value seek(Env *, Value, Value) const;
    Value set_encoding(Env *, Value, Value = nullptr);
    void set_fileno(int fileno) { m_fileno = fileno; }
    Value stat(Env *) const;
    static Value sysopen(Env *, Value, Value = nullptr, Value = nullptr);
    Value read(Env *, Value, Value) const;
    static Value read_file(Env *, Value, Value = nullptr, Value = nullptr);
    Value readbyte(Env *);
    Value readline(Env *) const;
    int rewind(Env *);
    int set_pos(Env *, Value);
    IoObject *to_io(Env *);
    static Value try_convert(Env *, Value);

    Value write(Env *, Args) const;
    static Value write_file(Env *, Value, Value);

    Value get_path() const;
    void set_path(StringObject *path) { m_path = path; }
    void set_path(String path) { m_path = new StringObject { path }; }

protected:
    void raise_if_closed(Env *) const;
    int write(Env *, Value) const;

private:
    EncodingObject *m_external_encoding { nullptr };
    EncodingObject *m_internal_encoding { nullptr };
    int m_fileno { -1 };
    bool m_closed { false };
    StringObject *m_path { nullptr };
};

}
