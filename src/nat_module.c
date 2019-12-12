#include "natalie.h"
#include "nat_module.h"
#include "nat_kernel.h"

NatObject *Module_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_module(env, NULL);
}

NatObject *Module_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_MODULE);
    NAT_ASSERT_ARGC(0);
    if (self->class_name) {
        return nat_string(env, self->class_name);
    } else {
        return Kernel_inspect(env, self, argc, args, kwargs, block);
    }
}
