#include "builtin.hpp"
#include "natalie.hpp"

extern char **environ;

namespace Natalie {

Value *ENV_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    Value *hash = hash_new(env);
    int i = 1;
    char *pair = *environ;
    for (; pair; i++) {
        char *eq = strchr(pair, '=');
        assert(eq);
        ssize_t index = eq - pair;
        Value *name = string(env, pair);
        name->str[index] = 0;
        name->str_len = index;
        hash_put(env, hash, name, string(env, getenv(name->str)));
        pair = *(environ + i);
    }
    return Hash_inspect(env, hash, 0, NULL, NULL);
}

Value *ENV_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *name = args[0];
    NAT_ASSERT_TYPE(name, NAT_VALUE_STRING, "String");
    char *value = getenv(name->str);
    if (value) {
        return string(env, value);
    } else {
        return NAT_NIL;
    }
}

Value *ENV_refeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    Value *name = args[0];
    Value *value = args[1];
    NAT_ASSERT_TYPE(name, NAT_VALUE_STRING, "String");
    NAT_ASSERT_TYPE(value, NAT_VALUE_STRING, "String");
    setenv(name->str, value->str, 1);
    return value;
}

}
