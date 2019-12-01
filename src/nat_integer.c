#include "natalie.h"
#include "nat_integer.h"

NatObject *Integer_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_INTEGER);
    char *str = int_to_string(self->integer);
    return nat_string(env, str);
}

NatObject *Integer_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_INTEGER);
    assert(argc == 1);
    NatObject* arg = args[0];
    assert(arg->type == NAT_VALUE_INTEGER);
    int64_t result = self->integer + arg->integer;
    return nat_integer(env, result);
}

NatObject *Integer_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_INTEGER);
    assert(argc == 1);
    NatObject* arg = args[0];
    assert(arg->type == NAT_VALUE_INTEGER);
    int64_t result = self->integer - arg->integer;
    return nat_integer(env, result);
}

NatObject *Integer_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_INTEGER);
    assert(argc == 1);
    NatObject* arg = args[0];
    assert(arg->type == NAT_VALUE_INTEGER);
    int64_t result = self->integer * arg->integer;
    return nat_integer(env, result);
}

NatObject *Integer_div(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_INTEGER);
    assert(argc == 1);
    NatObject* arg = args[0];
    assert(arg->type == NAT_VALUE_INTEGER);
    // FIXME: raise ZeroDivisionError if arg is zero
    int64_t result = self->integer / arg->integer;
    return nat_integer(env, result);
}
