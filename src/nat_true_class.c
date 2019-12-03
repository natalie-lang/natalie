#include "natalie.h"
#include "nat_true_class.h"

NatObject *TrueClass_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    printf("FIXME: this method should not be defined\n");
    abort();
}

NatObject *TrueClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_TRUE);
    return nat_string(env, "true");
}
