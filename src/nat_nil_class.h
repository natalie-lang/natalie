#ifndef __NAT_NIL_CLASS__
#define __NAT_NIL_CLASS__

NatObject *NilClass_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *NilClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *NilClass_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);

#endif
