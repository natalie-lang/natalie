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
        , m_fileno { fileno }
        , m_sync { fileno == STDERR_FILENO } { }

    virtual ~IoObject() override {
        if (m_fileno == STDIN_FILENO || m_fileno == STDOUT_FILENO || m_fileno == STDERR_FILENO)
            return;
        if (m_autoclose && !m_closed && m_fileno != -1) {
            m_closed = true;
            m_fileno = -1;
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
    Value autoclose(Env *, Value);
    static Value binread(Env *, Value, Value = nullptr, Value = nullptr);
    static Value binwrite(Env *, Args);
    Value binmode(Env *);
    Value close(Env *);
    static Value copy_stream(Env *, Value, Value, Value = nullptr, Value = nullptr);
    Value dup(Env *) const;
    Value each_byte(Env *, Block *);
    Value external_encoding() const { return m_external_encoding; }
    Value fcntl(Env *, Value, Value = nullptr);
    int fdatasync(Env *);
    int fileno() const;
    int fileno(Env *) const;
    int fsync(Env *);
    Value getbyte(Env *);
    Value gets(Env *, Value = nullptr, Value = nullptr, Value = nullptr);
    Value initialize(Env *, Args, Block * = nullptr);
    Value inspect() const;
    Value internal_encoding() const { return m_internal_encoding; }
    bool is_autoclose(Env *) const;
    bool is_binmode(Env *) const;
    bool is_closed() const { return m_closed; }
    bool is_close_on_exec(Env *) const;
    bool is_eof(Env *);
    bool isatty(Env *) const;
    int lineno(Env *) const;
    static Value pipe(Env *, Value, Value, Block *, ClassObject *);
    int pos(Env *);
    Value pread(Env *, Value, Value, Value = nullptr);
    Value puts(Env *, Args);
    void puts(Env *, Value);
    void putstr(Env *, StringObject *);
    void putary(Env *, ArrayObject *);
    Value print(Env *, Args);
    Value pwrite(Env *, Value, Value);
    Value seek(Env *, Value, Value);
    Value set_close_on_exec(Env *, Value);
    Value set_encoding(Env *, Value, Value = nullptr);
    void set_fileno(int fileno) { m_fileno = fileno; }
    Value set_lineno(Env *, Value);
    Value set_sync(Env *, Value);
    Value stat(Env *) const;
    static Value sysopen(Env *, Value, Value = nullptr, Value = nullptr);
    Value read(Env *, Value, Value);
    static Value read_file(Env *, Args);
    Value readbyte(Env *);
    Value readline(Env *, Value = nullptr, Value = nullptr, Value = nullptr);
    int rewind(Env *);
    int set_pos(Env *, Value);
    static Value select(Env *, Value, Value = nullptr, Value = nullptr, Value = nullptr);
    bool sync(Env *) const;
    Value sysread(Env *, Value, Value = nullptr);
    Value sysseek(Env *, Value, Value = nullptr);
    Value syswrite(Env *, Value);
    IoObject *to_io(Env *);
    static Value try_convert(Env *, Value);
    Value ungetbyte(Env *, Value);

    Value write(Env *, Args);
    static Value write_file(Env *, Args);

    Value get_path() const;
    void set_path(StringObject *path) { m_path = path; }
    void set_path(String path) { m_path = new StringObject { path }; }

protected:
    void raise_if_closed(Env *) const;
    int write(Env *, Value);

private:
    EncodingObject *m_external_encoding { nullptr };
    EncodingObject *m_internal_encoding { nullptr };
    int m_fileno { -1 };
    int m_lineno { 0 };
    bool m_closed { false };
    bool m_autoclose { true };
    bool m_sync { false };
    StringObject *m_path { nullptr };
    TM::String m_read_buffer {};
};

}
