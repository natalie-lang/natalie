#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Class_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    Value *superclass;
    if (argc == 1) {
        superclass = args[0];
        if (NAT_TYPE(superclass) != NAT_VALUE_CLASS) {
            NAT_RAISE(env, "TypeError", "superclass must be a Class (%s given)", NAT_OBJ_CLASS(superclass)->class_name);
        }
    } else {
        superclass = NAT_OBJECT;
    }
    Value *klass = subclass(env, superclass, NULL);
    if (block) {
        Env e;
        build_block_env(&e, &block->env, env);
        block->fn(&e, klass, 0, NULL, NULL);
    }
    return klass;
}

Value *Class_superclass(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_CLASS);
    NAT_ASSERT_ARGC(0);
    return self->superclass ? self->superclass : NAT_NIL;
}

}
