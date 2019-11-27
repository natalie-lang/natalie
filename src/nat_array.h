#ifndef __NAT_ARRAY__
#define __NAT_ARRAY__

#include "natalie.h"

NatObject *Array_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Array_ltlt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Array_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Array_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Array_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);

#endif
