#include "natalie.h"
#include "builtin.h"

NatObject *Class_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_MOST(1);
    NatObject *superclass;
    if (argc == 1) {
        superclass = args[0];
        if (NAT_TYPE(superclass) != NAT_VALUE_CLASS) {
            NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "TypeError"), "superclass must be a Class (%s given)", superclass->klass->class_name);
        }
    } else {
        superclass = NAT_OBJECT;
    }
    NatObject *klass = nat_subclass(env, superclass, NULL);
    if (block) {
        NatEnv e;
        nat_build_block_env(&e, &block->env, env);
        block->fn(&e, klass, 0, NULL, NULL, NULL);
    }
    return klass;
}

NatObject *Class_superclass(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return self->superclass ? self->superclass : NAT_NIL;
}
