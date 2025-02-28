#pragma once

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

#include <atomic>

#ifdef fileno
#undef fileno
#endif

namespace Natalie {

class IoObject : public Object {
public:
    IoObject()
        : Object { Object::Type::Io, GlobalEnv::the()->Object()->const_fetch("IO"_s).as_class() } { }

    IoObject(ClassObject *klass)
        : Object { Object::Type::Io, klass } { }

    IoObject(Type type, ClassObject *klass)
        : Object { type, klass } { }

    IoObject(int fileno)
        : Object { Object::Type::Io, GlobalEnv::the()->Object()->const_fetch("IO"_s).as_class() }
        , m_sync { fileno == STDERR_FILENO } {
        set_fileno(fileno);
    }

    virtual ~IoObject() override {
        if (m_fileno == STDIN_FILENO || m_fileno == STDOUT_FILENO || m_fileno == STDERR_FILENO)
            return;
        if (m_autoclose && !m_closed && m_fileno != -1) {
            ::close(m_fileno);
            m_closed = true;
            m_fileno = -1;
        }
    }

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_external_encoding);
        visitor.visit(m_internal_encoding);
        visitor.visit(m_path);
    }

    Value advise(Env *, Value, Optional<Value>, Optional<Value>);
    Value autoclose(Env *, Value);
    static Value binread(Env *, Value, Optional<Value> = {}, Optional<Value> = {});
    static Value binwrite(Env *, Args &&);
    Value binmode(Env *);
    Value close(Env *);
    static Value copy_stream(Env *, Value, Value, Optional<Value> = {}, Optional<Value> = {});
    Value dup(Env *) const;
    Value each_byte(Env *, Block *);
    Value fcntl(Env *, Value, Optional<Value> = {});
    int fdatasync(Env *);
    int fileno() const;
    int fileno(Env *) const;
    int fsync(Env *);
    Value getbyte(Env *);
    Value getc(Env *);
    Value gets(Env *, Optional<Value> = {}, Optional<Value> = {}, Optional<Value> = {});
    Value initialize(Env *, Args &&, Block * = nullptr);
    Value inspect() const;
    bool is_autoclose(Env *) const;
    bool is_binmode(Env *) const;
    bool is_closed() const { return m_closed; }
    bool is_close_on_exec(Env *) const;
    bool is_eof(Env *);
    bool is_nonblock(Env *) const;
    bool isatty(Env *) const;
    int lineno(Env *) const;
    Value ltlt(Env *, Value);
    Value pid(Env *) const;
    static Value pipe(Env *, Optional<Value>, Optional<Value>, Block *, ClassObject *);
    static Value popen(Env *, Args &&, Block *, ClassObject *);
    int pos(Env *);
    Value pread(Env *, Value, Value, Optional<Value> = {});
    Value putc(Env *, Value);
    Value puts(Env *, Args &&);
    void puts(Env *, Value);
    void putstr(Env *, StringObject *);
    void putary(Env *, ArrayObject *);
    Value print(Env *, Args &&);
    Value pwrite(Env *, Value, Value);
    Value seek(Env *, Value, Optional<Value>);
    Value set_close_on_exec(Env *, Value);
    Value set_encoding(Env *, Optional<EncodingObject *>, Optional<EncodingObject *>);
    Value set_encoding(Env *, Optional<Value>, Optional<Value> = {});
    void set_fileno(int fileno) { m_fileno = fileno; }
    Value set_lineno(Env *, Value);
    Value set_sync(Env *, Value);
    void set_nonblock(Env *, bool) const;
    Value stat(Env *) const;
    static Value sysopen(Env *, Value, Optional<Value> = {}, Optional<Value> = {});
    Value read(Env *, Optional<Value>, Optional<Value> = {});
    static Value read_file(Env *, Args &&);
    Value readbyte(Env *);
    Value readline(Env *, Optional<Value> = {}, Optional<Value> = {}, Optional<Value> = {});
    int rewind(Env *);
    int set_pos(Env *, Value);
    static Value select(Env *, Value, Optional<Value> = {}, Optional<Value> = {}, Optional<Value> = {});
    void select_read(Env *env, timeval *timeout = nullptr) const;
    bool sync(Env *) const;
    Value sysread(Env *, Value, Optional<Value> = {});
    Value sysseek(Env *, Value, Optional<Value> = {});
    Value syswrite(Env *, Value);
    IoObject *to_io(Env *);
    static Value try_convert(Env *, Value);
    Value ungetbyte(Env *, Value);
    Value ungetc(Env *, Value);
    Value wait(Env *, Args &&);
    Value wait_readable(Env *, Optional<Value> = {});
    Value wait_writable(Env *, Optional<Value> = {});

    Value write(Env *, Args &&);
    static Value write_file(Env *, Args &&);
    Value write_nonblock(Env *, Value, Optional<Value> = {});

    Value get_path() const;
    void set_path(StringObject *path) { m_path = path; }
    void set_path(String path) { m_path = new StringObject { path }; }

    Value external_encoding() const {
        if (!m_external_encoding)
            return Value::nil();
        return m_external_encoding;
    }

    Value internal_encoding() const {
        if (!m_internal_encoding) return Value::nil();
        return m_internal_encoding;
    }

    static void build_constants(Env *env, ClassObject *klass);

protected:
    void raise_if_closed(Env *) const;
    int write(Env *, Value);

private:
    static const nat_int_t WAIT_READABLE = 1;
    static const nat_int_t WAIT_PRIORITY = 2;
    static const nat_int_t WAIT_WRITABLE = 4;

    ssize_t blocking_read(Env *env, void *buf, int count) const;

    EncodingObject *m_external_encoding { nullptr };
    EncodingObject *m_internal_encoding { nullptr };
    int m_fileno { -1 };
    FILE *m_fileptr { nullptr };
    int m_pid { -1 };
    int m_lineno { 0 };
    std::atomic<bool> m_closed { false };
    bool m_autoclose { true };
    bool m_sync { false };
    StringObject *m_path { nullptr };
    TM::String m_read_buffer {};
};
}
