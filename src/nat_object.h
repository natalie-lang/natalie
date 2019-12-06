#ifndef __NAT_OBJECT__
#define __NAT_OBJECT__

NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
