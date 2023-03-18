#include "natalie.hpp"

#include <limits.h>
#include <math.h>
#include <unistd.h>

namespace Natalie {

Value IoObject::initialize(Env *env, Value file_number) {
    file_number->assert_type(env, Object::Type::Integer, "Integer");
    nat_int_t fileno = file_number->as_integer()->to_nat_int_t();
    assert(fileno >= INT_MIN && fileno <= INT_MAX);
    set_fileno(fileno);
    return this;
}

Value IoObject::read_file(Env *env, Value filename) {
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, { filename }, nullptr)->as_file();
    auto data = file->read(env, nullptr);
    file->close(env);
    return data;
}

Value IoObject::write_file(Env *env, Value filename, Value string) {
    Value flags = new StringObject { "w" };
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, { filename, flags }, nullptr)->as_file();
    int bytes_written = file->write(env, string);
    file->close(env);
    return Value::integer(bytes_written);
}

#define NAT_READ_BYTES 1024

Value IoObject::read(Env *env, Value count_value) const {
    if (m_closed)
        env->raise("IOError", "closed stream");
    size_t bytes_read;
    if (count_value) {
        count_value->assert_type(env, Object::Type::Integer, "Integer");
        int count = count_value->as_integer()->to_nat_int_t();
        char *buf = new char[count + 1];
        bytes_read = ::read(m_fileno, buf, count);
        if (bytes_read == 0) {
            delete[] buf;
            return NilObject::the();
        } else {
            Value result = new StringObject { buf, bytes_read };
            delete[] buf;
            return result;
        }
    }
    char buf[NAT_READ_BYTES + 1];
    bytes_read = ::read(m_fileno, buf, NAT_READ_BYTES);
    if (bytes_read == 0) {
        return new StringObject { "" };
    }
    StringObject *str = new StringObject { buf, bytes_read };
    while (1) {
        bytes_read = ::read(m_fileno, buf, NAT_READ_BYTES);
        if (bytes_read == 0) break;
        buf[bytes_read] = 0;
        str->append(buf, bytes_read);
    }
    return str;
}

Value IoObject::append(Env *env, Value obj) {
    if (is_closed())
        env->raise("IOError", "cannot read closed stream");
    if (!obj->is_string() && obj->respond_to(env, "to_str"_s))
        obj = obj.send(env, "to_s"_s);
    obj->assert_type(env, Object::Type::String, "String");
    ::write(m_fileno, obj->as_string()->c_str(), obj->as_string()->length());
    if (errno) env->raise_errno();
    return this;
}

int IoObject::write(Env *env, Value obj) const {
    if (is_closed())
        env->raise("IOError", "cannot write closed stream");
    if (obj->type() != Object::Type::String) {
        obj = obj.send(env, "to_s"_s);
    }
    obj->assert_type(env, Object::Type::String, "String");
    int result = ::write(m_fileno, obj->as_string()->c_str(), obj->as_string()->length());
    if (result == -1) {
        Value error_number = Value::integer(errno);
        auto SystemCallError = find_top_level_const(env, "SystemCallError"_s);
        ExceptionObject *error = SystemCallError.send(env, "exception"_s, { error_number })->as_exception();
        env->raise_exception(error);
    }
    return result;
}

Value IoObject::write(Env *env, Args args) const {
    args.ensure_argc_at_least(env, 1);
    int bytes_written = 0;
    for (size_t i = 0; i < args.size(); i++) {
        bytes_written += write(env, args[i]);
    }
    return Value::integer(bytes_written);
}

// NATFIXME: Make this spec compliant and maybe more performant?
Value IoObject::gets(Env *env) const {
    if (is_closed())
        env->raise("IOError", "cannot read closed stream");
    char buffer[NAT_READ_BYTES + 1];
    size_t index;
    for (index = 0; index < NAT_READ_BYTES; ++index) {
        if (::read(m_fileno, &buffer[index], 1) == 0)
            return NilObject::the();

        if (buffer[index] == '\n')
            break;
    }
    auto line = new StringObject { buffer, index + 1 };
    env->set_last_line(line);
    return line;
}

Value IoObject::puts(Env *env, Args args) const {
    if (args.size() == 0) {
        dprintf(m_fileno, "\n");
    } else {
        for (size_t i = 0; i < args.size(); i++) {
            Value str = args[i].send(env, "to_s"_s);
            str->assert_type(env, Object::Type::String, "String");
            dprintf(m_fileno, "%s\n", str->as_string()->c_str());
        }
    }
    return NilObject::the();
}

Value IoObject::print(Env *env, Args args) const {
    if (args.size() > 0) {
        auto fsep = env->output_file_separator();
        auto valid_fsep = !fsep->is_nil();
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0 && valid_fsep)
                write(env, fsep);
            write(env, args[i]);
        }
    } else {
        Value lastline = env->last_line();
        write(env, lastline);
    }
    auto rsep = env->output_record_separator();
    if (!rsep->is_nil()) write(env, rsep);
    return NilObject::the();
}

Value IoObject::close(Env *env) {
    if (m_closed)
        return NilObject::the();
    int result = ::close(m_fileno);
    if (result == -1) {
        Value error_number = Value::integer(errno);
        auto SystemCallError = find_top_level_const(env, "SystemCallError"_s);
        ExceptionObject *error = SystemCallError.send(env, "exception"_s, { error_number })->as_exception();
        env->raise_exception(error);
    } else {
        m_closed = true;
        return NilObject::the();
    }
}

Value IoObject::seek(Env *env, Value amount_value, Value whence_value) const {
    amount_value->assert_type(env, Object::Type::Integer, "Integer");
    int amount = amount_value->as_integer()->to_nat_int_t();
    int whence = 0;
    if (whence_value) {
        switch (whence_value->type()) {
        case Object::Type::Integer:
            whence = whence_value->as_integer()->to_nat_int_t();
            break;
        case Object::Type::Symbol: {
            SymbolObject *whence_sym = whence_value->as_symbol();
            if (whence_sym->string() == "SET") {
                whence = 0;
            } else if (whence_sym->string() == "CUR") {
                whence = 1;
            } else if (whence_sym->string() == "END") {
                whence = 2;
            } else {
                env->raise("TypeError", "no implicit conversion of Symbol into Integer");
            }
            break;
        }
        default:
            env->raise("TypeError", "no implicit conversion of {} into Integer", whence_value->klass()->inspect_str());
        }
    }
    int result = lseek(m_fileno, amount, whence);
    if (result == -1) {
        Value error_number = Value::integer(errno);
        ExceptionObject *error = find_top_level_const(env, "SystemCallError"_s).send(env, "exception"_s, { error_number })->as_exception();
        env->raise_exception(error);
    } else {
        return Value::integer(0);
    }
}

Value IoObject::stat(Env *env) const {
    struct stat sb;
    auto file_desc = fileno(); // current file descriptor
    int result = ::fstat(file_desc, &sb);
    if (result < 0) env->raise_errno();
    return new FileStatObject { sb };
}

}
