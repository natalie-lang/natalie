#include <errno.h>
#include <unistd.h>

#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *IO_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *obj = Object_new(env, self, argc, args, block);
    obj->type = NAT_VALUE_IO;
    return obj;
}

NatObject *IO_initialize(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1); // TODO: ruby accepts 1..2
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_INTEGER, "Integer");
    self->fileno = NAT_INT_VALUE(args[0]);
    return self;
}

NatObject *IO_fileno(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return integer(env, self->fileno);
}

#define NAT_READ_BYTES 1024

NatObject *IO_read(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1); // TODO: ruby accepts 0..2
    ssize_t bytes_read;
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], NAT_VALUE_INTEGER, "Integer");
        int count = NAT_INT_VALUE(args[0]);
        char *buf = static_cast<char *>(malloc((count + 1) * sizeof(char)));
        bytes_read = read(self->fileno, buf, count);
        if (bytes_read == 0) {
            free(buf);
            return NAT_NIL;
        } else {
            buf[bytes_read] = 0;
            NatObject *result = string(env, buf);
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
            NatObject *str = string(env, buf);
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

NatObject *IO_write(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    int bytes_written = 0;
    for (ssize_t i = 0; i < argc; i++) {
        NatObject *obj = args[i];
        if (NAT_TYPE(obj) != NAT_VALUE_STRING) {
            obj = send(env, obj, "to_s", 0, NULL, NULL);
        }
        NAT_ASSERT_TYPE(obj, NAT_VALUE_STRING, "String");
        ssize_t result = write(self->fileno, obj->str, obj->str_len);
        if (result == -1) {
            NatObject *error_number = integer(env, errno);
            NatObject *error = send(env, const_get(env, NAT_OBJECT, "SystemCallError", true), "exception", 1, &error_number, NULL);
            raise_exception(env, error);
            abort();
        } else {
            bytes_written += result;
        }
    }
    return integer(env, bytes_written);
}

NatObject *IO_puts(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    int fd = self->fileno;
    if (argc == 0) {
        dprintf(fd, "\n");
    } else {
        for (ssize_t i = 0; i < argc; i++) {
            NatObject *str = send(env, args[i], "to_s", 0, NULL, NULL);
            NAT_ASSERT_TYPE(str, NAT_VALUE_STRING, "String");
            dprintf(fd, "%s\n", str->str);
        }
    }
    return NAT_NIL;
}

NatObject *IO_print(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    int fd = self->fileno;
    if (argc > 0) {
        for (ssize_t i = 0; i < argc; i++) {
            NatObject *str = send(env, args[i], "to_s", 0, NULL, NULL);
            NAT_ASSERT_TYPE(str, NAT_VALUE_STRING, "String");
            dprintf(fd, "%s", str->str);
        }
    }
    return NAT_NIL;
}

NatObject *IO_close(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    int result = close(self->fileno);
    if (result == -1) {
        NatObject *error_number = integer(env, errno);
        NatObject *error = send(env, const_get(env, NAT_OBJECT, "SystemCallError", true), "exception", 1, &error_number, NULL);
        raise_exception(env, error);
        abort();
    } else {
        return NAT_NIL;
    }
}

NatObject *IO_seek(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC2(1, 2);
    NatObject *amount_obj = args[0];
    NAT_ASSERT_TYPE(amount_obj, NAT_VALUE_INTEGER, "Integer");
    int amount = NAT_INT_VALUE(amount_obj);
    int whence = 0;
    if (argc > 1) {
        NatObject *whence_obj = args[1];
        switch (NAT_TYPE(whence_obj)) {
        case NAT_VALUE_INTEGER:
            whence = NAT_INT_VALUE(whence_obj);
            break;
        case NAT_VALUE_SYMBOL:
            if (strcmp(whence_obj->symbol, "SET") == 0) {
                whence = 0;
            } else if (strcmp(whence_obj->symbol, "CUR") == 0) {
                whence = 1;
            } else if (strcmp(whence_obj->symbol, "END") == 0) {
                whence = 2;
            } else {
                NAT_RAISE(env, "TypeError", "no implicit conversion of Symbol into NAT_INTEGER");
            }
            break;
        default:
            NAT_RAISE(env, "TypeError", "no implicit conversion of %s into NAT_INTEGER", NAT_OBJ_CLASS(whence_obj)->class_name);
        }
    }
    int result = lseek(self->fileno, amount, whence);
    if (result == -1) {
        NatObject *error_number = integer(env, errno);
        NatObject *error = send(env, const_get(env, NAT_OBJECT, "SystemCallError", true), "exception", 1, &error_number, NULL);
        raise_exception(env, error);
        abort();
    } else {
        return integer(env, 0);
    }
}

}
