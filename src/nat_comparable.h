#ifndef __NAT_COMPARABLE__
#define __NAT_COMPARABLE__

#include "natalie.h"

#define COMPARABLE_INIT() \
    nat_define_method(Comparable, "==", Comparable_eqeq); \
    nat_define_method(Comparable, "!=", Comparable_neq); \
    nat_define_method(Comparable, "<", Comparable_lt); \
    nat_define_method(Comparable, ">", Comparable_gt);

NatObject *Comparable_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_neq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_lt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_gt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
