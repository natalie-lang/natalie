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
    Value args[] = { filename };
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, 1, args, nullptr)->as_file();
    auto data = file->read(env, nullptr);
    file->close(env);
    return data;
}

Value IoObject::write_file(Env *env, Value filename, Value string) {
    Value flags = new StringObject { "w" };
    Value args[] = { filename, flags };
    ClassObject *File = GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class();
    FileObject *file = _new(env, File, 2, args, nullptr)->as_file();
    auto bytes_written = file->write(env, 1, &string);
    file->close(env);
    return bytes_written;
}

#define NAT_READ_BYTES 1024

Value IoObject::read(Env *env, Value count_value) const {
    if (m_closed)
        env->raise("IOError", "closed stream");
    size_t bytes_read;
    if (count_value) {
        count_value->assert_type(env, Object::Type::Integer, "Integer");
        int count = count_value->as_integer()->to_nat_int_t();
        char *buf = static_cast<char *>(malloc((count + 1) * sizeof(char)));
        bytes_read = ::read(m_fileno, buf, count);
        if (bytes_read == 0) {
            free(buf);
            return NilObject::the();
        } else {
            buf[bytes_read] = 0;
            Value result = new StringObject { buf };
            free(buf);
            return result;
        }
    }
    char buf[NAT_READ_BYTES + 1];
    bytes_read = ::read(m_fileno, buf, NAT_READ_BYTES);
    if (bytes_read == 0) {
        return new StringObject { "" };
    }
    buf[bytes_read] = 0;
    StringObject *str = new StringObject { buf };
    while (1) {
        bytes_read = ::read(m_fileno, buf, NAT_READ_BYTES);
        if (bytes_read == 0) break;
        buf[bytes_read] = 0;
        str->append(env, buf);
    }
    return str;
}

Value IoObject::write(Env *env, size_t argc, Value *args) const {
    env->ensure_argc_at_least(argc, 1);
    int bytes_written = 0;
    for (size_t i = 0; i < argc; i++) {
        Value obj = args[i];
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
        } else {
            bytes_written += result;
        }
    }
    return Value::integer(bytes_written);
}

Value IoObject::puts(Env *env, size_t argc, Value *args) const {
    if (argc == 0) {
        dprintf(m_fileno, "\n");
    } else {
        for (size_t i = 0; i < argc; i++) {
            Value str = args[i].send(env, "to_s"_s);
            str->assert_type(env, Object::Type::String, "String");
            dprintf(m_fileno, "%s\n", str->as_string()->c_str());
        }
    }
    return NilObject::the();
}

Value IoObject::print(Env *env, size_t argc, Value *args) const {
    if (argc > 0) {
        for (size_t i = 0; i < argc; i++) {
            Value str = args[i].send(env, "to_s"_s);
            str->assert_type(env, Object::Type::String, "String");
            dprintf(m_fileno, "%s", str->as_string()->c_str());
        }
    }
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
            if (strcmp(whence_sym->c_str(), "SET") == 0) {
                whence = 0;
            } else if (strcmp(whence_sym->c_str(), "CUR") == 0) {
                whence = 1;
            } else if (strcmp(whence_sym->c_str(), "END") == 0) {
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

}
