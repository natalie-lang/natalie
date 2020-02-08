#include <unistd.h>

#include "natalie.h"
#include "nat_object.h"

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
    }
}
