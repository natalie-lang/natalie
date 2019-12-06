#ifndef __NAT_TRUE_CLASS__
#define __NAT_TRUE_CLASS__

NatObject *TrueClass_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *TrueClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
