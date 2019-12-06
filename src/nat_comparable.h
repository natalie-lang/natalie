#ifndef __NAT_COMPARABLE__
#define __NAT_COMPARABLE__

#include "natalie.h"

#define COMPARABLE_INIT() \
    nat_define_method(Comparable, "==", Comparable_eqeq); \
    nat_define_method(Comparable, "!=", Comparable_neq); \

NatObject *Comparable_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_neq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
