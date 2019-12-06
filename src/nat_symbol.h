#ifndef __NAT_SYMBOL__
#define __NAT_SYMBOL__

#include "natalie.h"

NatObject *Symbol_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Symbol_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
