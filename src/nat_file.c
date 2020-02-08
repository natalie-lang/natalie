#include <fcntl.h>
#include <errno.h>

#include "natalie.h"
#include "nat_io.h"

NatObject *File_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC2(1, 2); // TODO: Ruby accepts 3 args??
    assert(NAT_TYPE(args[0]) == NAT_VALUE_STRING);
    int fileno = open(args[0]->str, 0);
    if (fileno == -1) {
        if (errno == ENOENT) {
            NAT_RAISE(
                env,
                nat_const_get(env, nat_const_get(env, Object, "Errno"), "ENOENT"),
                "No such file or directory - %s",
                args[0]->str
            );
        } else {
            // TODO: dynamically build exception from errno
            printf("Unknown error: %d", errno);
            abort();
        }
    } else {
        self->fileno = fileno;
        return self;
    }
}
