#include <errno.h>
#include <unistd.h>

#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *IO_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *obj = new IoValue { env, self->as_class() };
    return obj->initialize(env, argc, args, block);
}

Value *IO_initialize(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1); // TODO: ruby accepts 1..2
    assert(args[0]->type() == Value::Type::Integer);
    //NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
    IoValue *self = self_value->as_io();
    self->set_fileno(args[0]->as_integer()->to_int64_t());
    return self;
}

Value *IO_fileno(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    IoValue *self = self_value->as_io();
    return new IntegerValue { env, self->fileno() };
}

#define NAT_READ_BYTES 1024

Value *IO_read(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1); // TODO: ruby accepts 0..2
    IoValue *self = self_value->as_io();
    ssize_t bytes_read;
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
        int count = args[0]->as_integer()->to_int64_t();
        char *buf = static_cast<char *>(malloc((count + 1) * sizeof(char)));
        bytes_read = read(self->fileno(), buf, count);
        if (bytes_read == 0) {
            free(buf);
            return NAT_NIL;
        } else {
            buf[bytes_read] = 0;
            Value *result = new StringValue { env, buf };
            free(buf);
            return result;
        }
    } else if (argc == 0) {
        char buf[NAT_READ_BYTES + 1];
        bytes_read = read(self->fileno(), buf, NAT_READ_BYTES);
        if (bytes_read == 0) {
            return new StringValue { env, "" };
        } else {
            buf[bytes_read] = 0;
            StringValue *str = new StringValue { env, buf };
            while (1) {
                bytes_read = read(self->fileno(), buf, NAT_READ_BYTES);
                if (bytes_read == 0) break;
                buf[bytes_read] = 0;
                str->append(env, buf);
            }
            return str;
        }
    } else {
        printf("TODO: 3 args passed to IO#read\n");
        abort();
    }
}

Value *IO_write(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    IoValue *self = self_value->as_io();
    int bytes_written = 0;
    for (ssize_t i = 0; i < argc; i++) {
        Value *obj = args[i];
        if (obj->type() != Value::Type::String) {
            obj = obj->send(env, "to_s");
        }
        NAT_ASSERT_TYPE(obj, Value::Type::String, "String");
        ssize_t result = write(self->fileno(), obj->as_string()->c_str(), obj->as_string()->length());
        if (result == -1) {
            Value *error_number = new IntegerValue { env, errno };
            ExceptionValue *error = NAT_OBJECT->const_get(env, "SystemCallError", true)->send(env, "exception", 1, &error_number, nullptr)->as_exception();
            env->raise_exception(error);
            abort();
        } else {
            bytes_written += result;
        }
    }
    return new IntegerValue { env, bytes_written };
}

Value *IO_puts(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IoValue *self = self_value->as_io();
    int fd = self->fileno();
    if (argc == 0) {
        dprintf(fd, "\n");
    } else {
        for (ssize_t i = 0; i < argc; i++) {
            Value *str = args[i]->send(env, "to_s");
            NAT_ASSERT_TYPE(str, Value::Type::String, "String");
            dprintf(fd, "%s\n", str->as_string()->c_str());
        }
    }
    return NAT_NIL;
}

Value *IO_print(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IoValue *self = self_value->as_io();
    int fd = self->fileno();
    if (argc > 0) {
        for (ssize_t i = 0; i < argc; i++) {
            Value *str = args[i]->send(env, "to_s");
            NAT_ASSERT_TYPE(str, Value::Type::String, "String");
            dprintf(fd, "%s", str->as_string()->c_str());
        }
    }
    return NAT_NIL;
}

Value *IO_close(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    IoValue *self = self_value->as_io();
    int result = close(self->fileno());
    if (result == -1) {
        Value *error_number = new IntegerValue { env, errno };
        ExceptionValue *error = NAT_OBJECT->const_get(env, "SystemCallError", true)->send(env, "exception", 1, &error_number, nullptr)->as_exception();
        env->raise_exception(error);
        abort();
    } else {
        return NAT_NIL;
    }
}

Value *IO_seek(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC2(1, 2);
    IoValue *self = self_value->as_io();
    Value *amount_obj = args[0];
    NAT_ASSERT_TYPE(amount_obj, Value::Type::Integer, "Integer");
    int amount = amount_obj->as_integer()->to_int64_t();
    int whence = 0;
    if (argc > 1) {
        Value *whence_obj = args[1];
        switch (whence_obj->type()) {
        case Value::Type::Integer:
            whence = whence_obj->as_integer()->to_int64_t();
            break;
        case Value::Type::Symbol: {
            SymbolValue *whence_sym = whence_obj->as_symbol();
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
            NAT_RAISE(env, "TypeError", "no implicit conversion of %s into Integer", whence_obj->klass()->class_name());
        }
    }
    int result = lseek(self->fileno(), amount, whence);
    if (result == -1) {
        Value *error_number = new IntegerValue { env, errno };
        ExceptionValue *error = NAT_OBJECT->const_get(env, "SystemCallError", true)->send(env, "exception", 1, &error_number, nullptr)->as_exception();
        env->raise_exception(error);
        abort();
    } else {
        return new IntegerValue { env, 0 };
    }
}

}
