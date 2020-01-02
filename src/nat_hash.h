#ifndef __NAT_HASH__
#define __NAT_HASH__

#include "natalie.h"

NatObject *Hash_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
