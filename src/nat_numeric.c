#include "natalie.h"
#include "nat_numeric.h"

NatObject *Numeric_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_NUMBER);
    char *str = long_long_to_string(self->number);
    return nat_string(env, str);
}

NatObject *Numeric_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_NUMBER);
    assert(argc == 1);
    NatObject* arg = args[0];
    assert(arg->type == NAT_VALUE_NUMBER);
    long long result = self->number * arg->number;
    return nat_number(env, result);
}
