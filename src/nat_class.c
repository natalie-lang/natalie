#include "natalie.h"
#include "nat_class.h"

NatObject *Class_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *Object = env_get(env, "Object");
    return nat_subclass(env, Object, NULL);
}

NatObject *Class_superclass(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return self->superclass ? self->superclass : env_get(env, "nil");
}
