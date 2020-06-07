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
    assert(args[0]->type == Value::Type::Integer);
    //NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
    IoValue *self = self_value->as_io();
    self->fileno = NAT_INT_VALUE(args[0]);
    return self;
}

Value *IO_fileno(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    IoValue *self = self_value->as_io();
    return integer(env, self->fileno);
}

#define NAT_READ_BYTES 1024

Value *IO_read(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1); // TODO: ruby accepts 0..2
    IoValue *self = self_value->as_io();
    ssize_t bytes_read;
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
        int count = NAT_INT_VALUE(args[0]);
        char *buf = static_cast<char *>(malloc((count + 1) * sizeof(char)));
        bytes_read = read(self->fileno, buf, count);
        if (bytes_read == 0) {
            free(buf);
            return NAT_NIL;
        } else {
            buf[bytes_read] = 0;
            Value *result = string(env, buf);
            free(buf);
            return result;
        }
    } else if (argc == 0) {
        char buf[NAT_READ_BYTES + 1];
        bytes_read = read(self->fileno, buf, NAT_READ_BYTES);
        if (bytes_read == 0) {
            return string(env, "");
        } else {
            buf[bytes_read] = 0;
            StringValue *str = string(env, buf);
            while (1) {
                bytes_read = read(self->fileno, buf, NAT_READ_BYTES);
                if (bytes_read == 0) break;
                buf[bytes_read] = 0;
                string_append(env, str, buf);
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
        if (NAT_TYPE(obj) != Value::Type::String) {
            obj = send(env, obj, "to_s", 0, NULL, NULL);
        }
        NAT_ASSERT_TYPE(obj, Value::Type::String, "String");
        ssize_t result = write(self->fileno, obj->as_string()->str, obj->as_string()->str_len);
        if (result == -1) {
            Value *error_number = integer(env, errno);
            ExceptionValue *error = send(env, NAT_OBJECT->const_get(env, "SystemCallError", true), "exception", 1, &error_number, NULL)->as_exception();
            env->raise_exception(error);
            abort();
        } else {
            bytes_written += result;
        }
    }
    return integer(env, bytes_written);
}

Value *IO_puts(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IoValue *self = self_value->as_io();
    int fd = self->fileno;
    if (argc == 0) {
        dprintf(fd, "\n");
    } else {
        for (ssize_t i = 0; i < argc; i++) {
            Value *str = send(env, args[i], "to_s", 0, NULL, NULL);
            NAT_ASSERT_TYPE(str, Value::Type::String, "String");
            dprintf(fd, "%s\n", str->as_string()->str);
        }
    }
    return NAT_NIL;
}

Value *IO_print(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IoValue *self = self_value->as_io();
    int fd = self->fileno;
    if (argc > 0) {
        for (ssize_t i = 0; i < argc; i++) {
            Value *str = send(env, args[i], "to_s", 0, NULL, NULL);
            NAT_ASSERT_TYPE(str, Value::Type::String, "String");
            dprintf(fd, "%s", str->as_string()->str);
        }
    }
    return NAT_NIL;
}

Value *IO_close(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    IoValue *self = self_value->as_io();
    int result = close(self->fileno);
    if (result == -1) {
        Value *error_number = integer(env, errno);
        ExceptionValue *error = send(env, NAT_OBJECT->const_get(env, "SystemCallError", true), "exception", 1, &error_number, NULL)->as_exception();
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
    int amount = NAT_INT_VALUE(amount_obj);
    int whence = 0;
    if (argc > 1) {
        Value *whence_obj = args[1];
        switch (NAT_TYPE(whence_obj)) {
        case Value::Type::Integer:
            whence = NAT_INT_VALUE(whence_obj);
            break;
        case Value::Type::Symbol: {
            SymbolValue *whence_sym = whence_obj->as_symbol();
            if (strcmp(whence_sym->symbol, "SET") == 0) {
                whence = 0;
            } else if (strcmp(whence_sym->symbol, "CUR") == 0) {
                whence = 1;
            } else if (strcmp(whence_sym->symbol, "END") == 0) {
                whence = 2;
            } else {
                NAT_RAISE(env, "TypeError", "no implicit conversion of Symbol into NAT_INTEGER");
            }
            break;
        }
        default:
            NAT_RAISE(env, "TypeError", "no implicit conversion of %s into NAT_INTEGER", NAT_OBJ_CLASS(whence_obj)->class_name);
        }
    }
    int result = lseek(self->fileno, amount, whence);
    if (result == -1) {
        Value *error_number = integer(env, errno);
        ExceptionValue *error = send(env, NAT_OBJECT->const_get(env, "SystemCallError", true), "exception", 1, &error_number, NULL)->as_exception();
        env->raise_exception(error);
        abort();
    } else {
        return integer(env, 0);
    }
}

}
