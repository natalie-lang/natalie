#include "natalie.h"
#include "nat_hash.h"

NatObject *Hash_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(self->type == NAT_VALUE_HASH);
    NatObject *out = nat_string(env, "{");
    struct hashmap_iter *iter;
    NatObject *key, *val;
    size_t last_index = self->hashmap.num_entries - 1;
    size_t index;
    for (iter = hashmap_iter(&self->hashmap), index = 0; iter; iter = hashmap_iter_next(&self->hashmap, iter), index++) {
        key = (NatObject*)hashmap_iter_get_key(iter);
        NatObject *key_repr = nat_send(env, key, "inspect", 0, NULL, NULL);
        nat_string_append(out, key_repr->str);
        nat_string_append(out, "=>");
        val = (NatObject*)hashmap_get(&self->hashmap, key);
        NatObject *val_repr = nat_send(env, val, "inspect", 0, NULL, NULL);
        nat_string_append(out, val_repr->str);
        if (index < last_index) {
            nat_string_append(out, ", ");
        }
    }
    nat_string_append_char(out, '}');
    return out;
}
