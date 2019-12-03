#ifndef __NAT_OBJECT__
#define __NAT_OBJECT__

NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Object_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Object_object_id(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
NatObject *Object_equal(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);

#endif
