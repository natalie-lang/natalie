#ifndef __NAT_NUMERIC__
#define __NAT_NUMERIC__

#include "natalie.h"

NatObject *Numeric_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Numeric_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);

#endif
