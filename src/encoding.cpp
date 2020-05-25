#include "builtin.hpp"
#include "natalie.hpp"

NatObject *Encoding_list(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NatObject *ary = nat_array(env);
    nat_array_push(env, ary, nat_const_get(env, self, "ASCII_8BIT", true));
    nat_array_push(env, ary, nat_const_get(env, self, "UTF_8", true));
    return ary;
}

NatObject *Encoding_inspect(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return nat_sprintf(env, "#<Encoding:%S>", nat_vector_get(&self->encoding_names->ary, 0));
}

NatObject *Encoding_name(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return static_cast<NatObject *>(nat_vector_get(&self->encoding_names->ary, 0));
}

NatObject *Encoding_names(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return self->encoding_names;
}
