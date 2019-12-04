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

NatObject *Integer_cmp(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_INTEGER);
    assert(argc == 1);
    NatObject* arg = args[0];
    if (arg->type != NAT_VALUE_INTEGER) return env_get(env, "nil");
    if (self->integer < arg->integer) {
        return nat_integer(env, -1);
    } else if (self->integer == arg->integer) {
        return nat_integer(env, 0);
    } else {
        return nat_integer(env, 1);
    }
}

NatObject *Integer_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_INTEGER);
    assert(argc == 1);
    NatObject* arg = args[0];
    if (arg->type == NAT_VALUE_INTEGER && self->integer == arg->integer) {
        return env_get(env, "true");
    } else {
        return env_get(env, "false");
    }
}
