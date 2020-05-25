#include "builtin.hpp"
#include "natalie.hpp"

NatObject *Class_new(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0, 1);
    NatObject *superclass;
    if (argc == 1) {
        superclass = args[0];
        if (NAT_TYPE(superclass) != NAT_VALUE_CLASS) {
            NAT_RAISE(env, "TypeError", "superclass must be a Class (%s given)", NAT_OBJ_CLASS(superclass)->class_name);
        }
    } else {
        superclass = NAT_OBJECT;
    }
    NatObject *klass = nat_subclass(env, superclass, NULL);
    if (block) {
        NatEnv e;
        nat_build_block_env(&e, &block->env, env);
        block->fn(&e, klass, 0, NULL, NULL);
    }
    return klass;
}

NatObject *Class_superclass(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return self->superclass ? self->superclass : NAT_NIL;
}
