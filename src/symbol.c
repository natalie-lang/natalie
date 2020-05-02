#include "builtin.h"
#include "natalie.h"

NatObject *Symbol_to_s(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_SYMBOL);
    NAT_ASSERT_ARGC(0);
    return nat_string(env, self->symbol);
}

NatObject *Symbol_inspect(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_SYMBOL);
    NAT_ASSERT_ARGC(0);
    NatObject *str = nat_string(env, ":");
    ssize_t len = strlen(self->symbol);
    for (ssize_t i = 0; i < len; i++) {
        char c = self->symbol[i];
        if (!((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == '_')) {
            nat_string_append_nat_string(env, str, nat_send(env, nat_string(env, self->symbol), "inspect", 0, NULL, NULL));
            return str;
        }
    };
    nat_string_append(env, str, self->symbol);
    return str;
}

NatObject *Symbol_to_proc(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_SYMBOL);
    NAT_ASSERT_ARGC(0);
    NatEnv *block_env = nat_build_detached_block_env(env);
    nat_var_set(block_env, "name", 0, true, self);
    NatBlock *proc_block = nat_block(block_env, self, Symbol_to_proc_block_fn);
    return nat_proc(env, proc_block);
}

NatObject *Symbol_to_proc_block_fn(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *name_obj = nat_var_get(env->outer, "name", 0);
    assert(name_obj);
    char *name = name_obj->symbol;
    return nat_send(env, args[0], name, 0, NULL, NULL);
}
