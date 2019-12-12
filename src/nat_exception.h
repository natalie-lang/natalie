#ifndef __NAT_EXCEPTION__
#define __NAT_EXCEPTION__

#include "natalie.h"

NatObject *Exception_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
