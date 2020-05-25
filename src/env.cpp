#include "builtin.hpp"
#include "natalie.hpp"

extern char **environ;

NatObject *ENV_inspect(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    NatObject *hash = nat_hash(env);
    int i = 1;
    char *pair = *environ;
    for (; pair; i++) {
        char *eq = strchr(pair, '=');
        assert(eq);
        ssize_t index = eq - pair;
        NatObject *name = nat_string(env, pair);
        name->str[index] = 0;
        name->str_len = index;
        nat_hash_put(env, hash, name, nat_string(env, getenv(name->str)));
        pair = *(environ + i);
    }
    return Hash_inspect(env, hash, 0, NULL, NULL);
}

NatObject *ENV_ref(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *name = args[0];
    NAT_ASSERT_TYPE(name, NAT_VALUE_STRING, "String");
    char *value = getenv(name->str);
    if (value) {
        return nat_string(env, value);
    } else {
        return NAT_NIL;
    }
}

NatObject *ENV_refeq(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(2);
    NatObject *name = args[0];
    NatObject *value = args[1];
    NAT_ASSERT_TYPE(name, NAT_VALUE_STRING, "String");
    NAT_ASSERT_TYPE(value, NAT_VALUE_STRING, "String");
    setenv(name->str, value->str, 1);
    return value;
}
