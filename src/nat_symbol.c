#include "natalie.h"
#include "nat_symbol.h"

NatObject *Symbol_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_SYMBOL);
    return nat_string(env, self->symbol);
}

NatObject *Symbol_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_SYMBOL);
    NatObject *str = nat_string(env, ":");
    nat_string_append(str, self->symbol);
    return str;
}
