#include "natalie.h"
#include "nat_proc.h"

NatObject *Proc_call(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_PROC);
    return nat_run_block(env, self->block, argc, args, kwargs, block);
}

NatObject *Proc_lambda(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_PROC);
    if (self->lambda) {
        return true_obj;
    } else {
        return false_obj;
    }
}
