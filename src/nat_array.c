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
