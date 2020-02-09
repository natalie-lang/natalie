#include <fcntl.h>
#include <errno.h>

#include "natalie.h"
#include "nat_io.h"

NatObject *File_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC2(1, 2); // TODO: Ruby accepts 3 args??
    NatObject *filename = args[0];
    assert(NAT_TYPE(filename) == NAT_VALUE_STRING);
    int fileno = open(filename->str, 0);
    if (fileno == -1) {
        NatObject **exception_args = calloc(2, sizeof(NatObject*));
        exception_args[0] = filename;
        exception_args[1] = nat_integer(env, errno);
        NatObject *error = nat_send(env, nat_const_get(env, Object, "SystemCallError"), "exception", 2, exception_args, NULL);
        nat_raise_exception(env, error);
    } else {
        self->fileno = fileno;
        return self;
    }
}
