#include "natalie.h"
#include "nat_nil_class.h"

NatObject *NilClass_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    printf("FIXME: this method should not be defined\n");
    abort();
}

NatObject *NilClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_NIL);
    NAT_ASSERT_ARGC(0);
    NatObject *out = nat_string(env, "");
    return out;
}

NatObject *NilClass_to_a(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_NIL);
    NAT_ASSERT_ARGC(0);
    return nat_array(env);
}

NatObject *NilClass_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_NIL);
    NAT_ASSERT_ARGC(0);
    NatObject *out = nat_string(env, "nil");
    return out;
}
