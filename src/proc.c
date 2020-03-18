#include "natalie.h"
#include "builtin.h"

NatObject *Proc_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    return nat_proc(env, block);
}

NatObject *Proc_call(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_PROC);
    return NAT_RUN_BLOCK_WITHOUT_BREAK(env, self->block, argc, args, kwargs, block);
}

NatObject *Proc_lambda(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_PROC);
    if (self->lambda) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}
