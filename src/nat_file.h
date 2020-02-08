#ifndef __NAT_FILE__
#define __NAT_FILE__

#include "natalie.h"

NatObject *File_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
