#include "builtin.h"
#include "natalie.h"

NatObject *Symbol_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_SYMBOL);
    NAT_ASSERT_ARGC(0);
    return nat_string(env, self->symbol);
}

NatObject *Symbol_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_SYMBOL);
    NAT_ASSERT_ARGC(0);
    NatObject *str = nat_string(env, ":");
    size_t len = strlen(self->symbol);
    for (size_t i = 0; i < len; i++) {
        char c = self->symbol[i];
        if (!((c >= 65 && c <= 90) || (c >= 97 && c <= 122))) {
            nat_string_append_nat_string(env, str, nat_send(env, nat_string(env, self->symbol), "inspect", 0, NULL, NULL));
            return str;
        }
    };
    nat_string_append(env, str, self->symbol);
    return str;
}
