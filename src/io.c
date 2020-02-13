#include <unistd.h>
#include <errno.h>

#include "natalie.h"
#include "builtin.h"

NatObject *IO_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *obj = Object_new(env, self, argc, args, kwargs, block);
    obj->type = NAT_VALUE_IO;
    return obj;
}

NatObject *IO_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1); // TODO: ruby accepts 1..2
    assert(NAT_TYPE(args[0]) == NAT_VALUE_INTEGER);
    self->fileno = NAT_INT_VALUE(args[0]);
    return self;
}

NatObject *IO_fileno(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_integer(env, self->fileno);
}

NatObject *IO_read(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC2(0, 1); // TODO: ruby accepts 0..2
    ssize_t bytes_read;
    if (argc == 1) {
        assert(NAT_TYPE(args[0]) == NAT_VALUE_INTEGER);
        int count = NAT_INT_VALUE(args[0]);
        char *buf = malloc(count * sizeof(char));
        bytes_read = read(self->fileno, buf, count);
        if (bytes_read == 0) {
            return nil;
        } else {
            return nat_string(env, buf);
        }
    } else if (argc == 0) {
        char *buf = malloc(1024 * sizeof(char));
        bytes_read = read(self->fileno, buf, 1024);
        if (bytes_read == 0) {
            return nat_string(env, "");
        } else {
            NatObject *str = nat_string(env, buf);
            while (1) {
                bytes_read = read(self->fileno, buf, 1024);
                if (bytes_read == 0) break;
                nat_string_append(str, buf);
            }
            return str;
        }
    } else {
        printf("TODO: 3 args passed to IO#read\n");
        abort();
    }
}

NatObject *IO_write(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    int bytes_written = 0;
    for (size_t i=0; i<argc; i++) {
        char *buf;
        size_t len;
        NatObject *obj = args[i];
        if (NAT_TYPE(obj) != NAT_VALUE_STRING) {
            obj = nat_send(env, obj, "to_s", 0, NULL, NULL);
        }
        assert(NAT_TYPE(obj) == NAT_VALUE_STRING);
        ssize_t result = write(self->fileno, obj->str, obj->str_len);
        if (result == -1) {
            NatObject *error_number = nat_integer(env, errno);
            NatObject *error = nat_send(env, nat_const_get(env, Object, "SystemCallError"), "exception", 1, &error_number, NULL);
            nat_raise_exception(env, error);
            abort();
        } else {
            bytes_written += result;
        }
    }
    return nat_integer(env, bytes_written);
}

NatObject *IO_close(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    int result = close(self->fileno);
    if (result == -1) {
        NatObject *error_number = nat_integer(env, errno);
        NatObject *error = nat_send(env, nat_const_get(env, Object, "SystemCallError"), "exception", 1, &error_number, NULL);
        nat_raise_exception(env, error);
        abort();
    } else {
        return nil;
    }
}

NatObject *IO_seek(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC2(1, 2);
    NatObject *amount_obj = args[0];
    assert(NAT_TYPE(amount_obj) == NAT_VALUE_INTEGER);
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
                    NAT_RAISE(env, nat_const_get(env, Object, "TypeError"), "no implicit conversion of Symbol into Integer");
                }
                break;
            default:
                NAT_RAISE(env, nat_const_get(env, Object, "TypeError"), "no implicit conversion of %s into Integer", whence_obj->klass->class_name);
        }
    }
    int result = lseek(self->fileno, amount, whence);
    if (result == -1) {
        NatObject *error_number = nat_integer(env, errno);
        NatObject *error = nat_send(env, nat_const_get(env, Object, "SystemCallError"), "exception", 1, &error_number, NULL);
        nat_raise_exception(env, error);
        abort();
    } else {
        return nat_integer(env, 0);
    }
}
