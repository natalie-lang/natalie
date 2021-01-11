#include "natalie.hpp"

#include <limits.h>
#include <math.h>
#include <unistd.h>

namespace Natalie {

ValuePtr IoValue::initialize(Env *env, ValuePtr file_number) {
    file_number->assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t fileno = file_number->as_integer()->to_nat_int_t();
    assert(fileno >= INT_MIN && fileno <= INT_MAX);
    set_fileno(fileno);
    return this;
}

ValuePtr IoValue::read_file(Env *env, ValuePtr filename) {
    ValuePtr args[] = { filename };
    FileValue *file = _new(env, env->Object()->const_fetch("File")->as_class(), 1, args, nullptr)->as_file();
    auto data = file->read(env, nullptr);
    file->close(env);
    return data;
}

#define NAT_READ_BYTES 1024

ValuePtr IoValue::read(Env *env, ValuePtr count_value) {
    if (m_closed)
        env->raise("IOError", "closed stream");
    size_t bytes_read;
    if (count_value) {
        count_value->assert_type(env, Value::Type::Integer, "Integer");
        int count = count_value->as_integer()->to_nat_int_t();
        char *buf = static_cast<char *>(GC_MALLOC((count + 1) * sizeof(char)));
        bytes_read = ::read(m_fileno, buf, count);
        if (bytes_read == 0) {
            GC_FREE(buf);
            return env->nil_obj();
        } else {
            buf[bytes_read] = 0;
            ValuePtr result = new StringValue { env, buf };
            GC_FREE(buf);
            return result;
        }
    }
    char buf[NAT_READ_BYTES + 1];
    bytes_read = ::read(m_fileno, buf, NAT_READ_BYTES);
    if (bytes_read == 0) {
        return new StringValue { env, "" };
    }
    buf[bytes_read] = 0;
    StringValue *str = new StringValue { env, buf };
    while (1) {
        bytes_read = ::read(m_fileno, buf, NAT_READ_BYTES);
        if (bytes_read == 0) break;
        buf[bytes_read] = 0;
        str->append(env, buf);
    }
    return str;
}

ValuePtr IoValue::write(Env *env, size_t argc, ValuePtr *args) {
    env->assert_argc_at_least(argc, 1);
    int bytes_written = 0;
    for (size_t i = 0; i < argc; i++) {
        ValuePtr obj = args[i];
        if (obj->type() != Value::Type::String) {
            obj = obj->send(env, "to_s");
        }
        obj->assert_type(env, Value::Type::String, "String");
        int result = ::write(m_fileno, obj->as_string()->c_str(), obj->as_string()->length());
        if (result == -1) {
            ValuePtr error_number = new IntegerValue { env, errno };
            ExceptionValue *error = env->Object()->const_find(env, "SystemCallError")->send(env, "exception", 1, &error_number, nullptr)->as_exception();
            env->raise_exception(error);
        } else {
            bytes_written += result;
        }
    }
    return new IntegerValue { env, bytes_written };
}

ValuePtr IoValue::puts(Env *env, size_t argc, ValuePtr *args) {
    if (argc == 0) {
        dprintf(m_fileno, "\n");
    } else {
        for (size_t i = 0; i < argc; i++) {
            ValuePtr str = args[i]->send(env, "to_s");
            str->assert_type(env, Value::Type::String, "String");
            dprintf(m_fileno, "%s\n", str->as_string()->c_str());
        }
    }
    return env->nil_obj();
}

ValuePtr IoValue::print(Env *env, size_t argc, ValuePtr *args) {
    if (argc > 0) {
        for (size_t i = 0; i < argc; i++) {
            ValuePtr str = args[i]->send(env, "to_s");
            str->assert_type(env, Value::Type::String, "String");
            dprintf(m_fileno, "%s", str->as_string()->c_str());
        }
    }
    return env->nil_obj();
}

ValuePtr IoValue::close(Env *env) {
    if (m_closed)
        return env->nil_obj();
    int result = ::close(m_fileno);
    if (result == -1) {
        ValuePtr error_number = new IntegerValue { env, errno };
        ExceptionValue *error = env->Object()->const_find(env, "SystemCallError")->send(env, "exception", 1, &error_number, nullptr)->as_exception();
        env->raise_exception(error);
    } else {
        m_closed = true;
        return env->nil_obj();
    }
}

ValuePtr IoValue::seek(Env *env, ValuePtr amount_value, ValuePtr whence_value) {
    amount_value->assert_type(env, Value::Type::Integer, "Integer");
    int amount = amount_value->as_integer()->to_nat_int_t();
    int whence = 0;
    if (whence_value) {
        switch (whence_value->type()) {
        case Value::Type::Integer:
            whence = whence_value->as_integer()->to_nat_int_t();
            break;
        case Value::Type::Symbol: {
            SymbolValue *whence_sym = whence_value->as_symbol();
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
            env->raise("TypeError", "no implicit conversion of %s into Integer", whence_value->klass()->class_name());
        }
    }
    int result = lseek(m_fileno, amount, whence);
    if (result == -1) {
        ValuePtr error_number = new IntegerValue { env, errno };
        ExceptionValue *error = env->Object()->const_find(env, "SystemCallError")->send(env, "exception", 1, &error_number, nullptr)->as_exception();
        env->raise_exception(error);
    } else {
        return new IntegerValue { env, 0 };
    }
}

}
