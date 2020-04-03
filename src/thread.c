#include "builtin.h"
#include "natalie.h"

NatObject *Thread_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK();
    return nat_thread(env, block);
}

NatObject *Thread_current(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_current_thread(env);
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

static NatObject *thread_id_to_nat_string(NatEnv *env, pthread_t tid) {
    unsigned char *ptc = (unsigned char *)(void *)(&tid);
    NatObject *str = nat_string(env, "0x");
    for (size_t i = 0; i < sizeof(tid); i++) {
        char buf[3];
        snprintf(buf, 3, "%02x", (unsigned)(ptc[i]));
        nat_string_append(env, str, buf);
    }
    return str;
}

NatObject *Thread_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_THREAD);
    NAT_ASSERT_ARGC(0);
    return nat_sprintf(env, "#<Thread:%S run>", thread_id_to_nat_string(env, self->thread_id));
}
