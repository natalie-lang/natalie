#include "natalie.h"
#include "nat_array.h"

NatObject *Array_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_ARRAY);
    NatObject *out = nat_string(env, "[");
    for (size_t i=0; i<self->ary_len; i++) {
        NatObject *obj = self->ary[i];
        NatObject *repr = nat_send(env, obj, "inspect", 0, NULL);
        nat_string_append(out, repr->str);
        if (i < self->ary_len-1) {
            nat_string_append(out, ", ");
        }
    }
    nat_string_append_char(out, ']');
    return out;
}

NatObject *Array_ltlt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(argc == 1);
    NatObject *arg = args[0];
    nat_array_push(self, arg);
    return self;
}

NatObject *Array_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(argc == 1);
    assert(self->type == NAT_VALUE_ARRAY);
    NatObject *arg = args[0];
    assert(arg->type == NAT_VALUE_ARRAY);
    NatObject *new = nat_array(env);
    nat_grow_array_at_least(new, self->ary_len + arg->ary_len);
    memcpy(new->ary, self->ary, self->ary_len * sizeof(NatObject*));
    memcpy(&new->ary[self->ary_len], arg->ary, arg->ary_len * sizeof(NatObject*));
    new->ary_len = self->ary_len + arg->ary_len;
    return new;
}
