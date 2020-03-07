#include "natalie.h"
#include "builtin.h"

NatObject *Thread_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK();
    return nat_thread(env, block);
}

NatObject *Thread_join(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_THREAD);
    NAT_ASSERT_ARGC(0);
    nat_thread_join(env, self);
    return self;
}

NatObject *Thread_value(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_THREAD);
    NAT_ASSERT_ARGC(0);
    return nat_thread_join(env, self);
}
