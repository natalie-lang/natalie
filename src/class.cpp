#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Class_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
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
    NatObject *klass = subclass(env, superclass, NULL);
    if (block) {
        Env e;
        build_block_env(&e, &block->env, env);
        block->fn(&e, klass, 0, NULL, NULL);
    }
    return klass;
}

NatObject *Class_superclass(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return self->superclass ? self->superclass : NAT_NIL;
}

}
