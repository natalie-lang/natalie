#ifndef __NAT_INTEGER__
#define __NAT_INTEGER__

#include "natalie.h"

NatObject *Integer_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Integer_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Integer_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Integer_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Integer_div(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Integer_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);

#endif
