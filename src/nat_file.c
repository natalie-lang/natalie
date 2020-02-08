#include <fcntl.h>

#include "natalie.h"
#include "nat_io.h"

NatObject *File_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC2(1, 2); // TODO: Ruby accepts 3 args??
    assert(NAT_TYPE(args[0]) == NAT_VALUE_STRING);
    self->fileno = open(args[0]->str, 0);
    return self;
}
