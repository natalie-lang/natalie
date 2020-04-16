#include "builtin.h"
#include "natalie.h"

NatObject *Encoding_list(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *ary = nat_array(env);
    nat_array_push(env, ary, nat_const_get(env, self, "ASCII_8BIT", true));
    nat_array_push(env, ary, nat_const_get(env, self, "UTF_8", true));
    return ary;
}

NatObject *Encoding_name(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return self->encoding_names->ary[0];
}

NatObject *Encoding_names(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return self->encoding_names;
}
