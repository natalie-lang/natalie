#include "natalie.hpp"

#include <limits.h>
#include <math.h>
#include <unistd.h>

namespace Natalie {

Value *IoValue::initialize(Env *env, Value *file_number) {
    NAT_ASSERT_TYPE(file_number, Value::Type::Integer, "Integer");
    int64_t fileno = file_number->as_integer()->to_int64_t();
    assert(fileno >= INT_MIN && fileno <= INT_MAX);
    set_fileno(fileno);
    return this;
}

#define NAT_READ_BYTES 1024

Value *IoValue::read(Env *env, Value *count_value) {
    ssize_t bytes_read;
    if (count_value) {
        NAT_ASSERT_TYPE(count_value, Value::Type::Integer, "Integer");
        int count = count_value->as_integer()->to_int64_t();
        char *buf = static_cast<char *>(GC_MALLOC((count + 1) * sizeof(char)));
        bytes_read = ::read(m_fileno, buf, count);
        if (bytes_read == 0) {
            GC_FREE(buf);
            return env->nil_obj();
        } else {
            buf[bytes_read] = 0;
            Value *result = new StringValue { env, buf };
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

Value *IoValue::write(Env *env, ssize_t argc, Value **args) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    int bytes_written = 0;
    for (ssize_t i = 0; i < argc; i++) {
        Value *obj = args[i];
        if (obj->type() != Value::Type::String) {
            obj = obj->send(env, "to_s");
        }
        NAT_ASSERT_TYPE(obj, Value::Type::String, "String");
        ssize_t result = ::write(m_fileno, obj->as_string()->c_str(), obj->as_string()->length());
        if (result == -1) {
            Value *error_number = new IntegerValue { env, errno };
            ExceptionValue *error = env->Object()->const_find(env, "SystemCallError")->send(env, "exception", 1, &error_number, nullptr)->as_exception();
            env->raise_exception(error);
            abort();
        } else {
            bytes_written += result;
        }
    }
    return new IntegerValue { env, bytes_written };
}

Value *IoValue::puts(Env *env, ssize_t argc, Value **args) {
    if (argc == 0) {
        dprintf(m_fileno, "\n");
    } else {
        for (ssize_t i = 0; i < argc; i++) {
            Value *str = args[i]->send(env, "to_s");
            NAT_ASSERT_TYPE(str, Value::Type::String, "String");
            dprintf(m_fileno, "%s\n", str->as_string()->c_str());
        }
    }
    return env->nil_obj();
}

Value *IoValue::print(Env *env, ssize_t argc, Value **args) {
    if (argc > 0) {
        for (ssize_t i = 0; i < argc; i++) {
            Value *str = args[i]->send(env, "to_s");
            NAT_ASSERT_TYPE(str, Value::Type::String, "String");
            dprintf(m_fileno, "%s", str->as_string()->c_str());
        }
    }
    return env->nil_obj();
}

Value *IoValue::close(Env *env) {
    int result = ::close(m_fileno);
    if (result == -1) {
        Value *error_number = new IntegerValue { env, errno };
        ExceptionValue *error = env->Object()->const_find(env, "SystemCallError")->send(env, "exception", 1, &error_number, nullptr)->as_exception();
        env->raise_exception(error);
        abort();
    } else {
        return env->nil_obj();
    }
}

Value *IoValue::seek(Env *env, Value *amount_value, Value *whence_value) {
    NAT_ASSERT_TYPE(amount_value, Value::Type::Integer, "Integer");
    int amount = amount_value->as_integer()->to_int64_t();
    int whence = 0;
    if (whence_value) {
        switch (whence_value->type()) {
        case Value::Type::Integer:
            whence = whence_value->as_integer()->to_int64_t();
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
                NAT_RAISE(env, "TypeError", "no implicit conversion of Symbol into Integer");
            }
            break;
        }
        default:
            NAT_RAISE(env, "TypeError", "no implicit conversion of %s into Integer", whence_value->klass()->class_name());
        }
    }
    int result = lseek(m_fileno, amount, whence);
    if (result == -1) {
        Value *error_number = new IntegerValue { env, errno };
        ExceptionValue *error = env->Object()->const_find(env, "SystemCallError")->send(env, "exception", 1, &error_number, nullptr)->as_exception();
        env->raise_exception(error);
        abort();
    } else {
        return new IntegerValue { env, 0 };
    }
}

}
