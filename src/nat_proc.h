#ifndef __NAT_PROC__
#define __NAT_PROC__

#include "natalie.h"

NatObject *Proc_call(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
