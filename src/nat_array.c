#include "natalie.h"
#include "nat_array.h"

NatObject *Array_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_ARRAY);
    NatObject *out = nat_string(env, "[");
    for (size_t i=0; i<self->ary_len; i++) {
        NatObject *obj = self->ary[i];
        NatObject *repr = nat_send(env, obj, "inspect", 0, NULL, NULL);
        nat_string_append(out, repr->str);
        if (i < self->ary_len-1) {
            nat_string_append(out, ", ");
        }
    }
    nat_string_append_char(out, ']');
    return out;
}

NatObject *Array_ltlt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(argc == 1);
    NatObject *arg = args[0];
    nat_array_push(self, arg);
    return self;
}

NatObject *Array_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
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

NatObject *Array_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(argc == 1 || argc == 2);
    assert(self->type == NAT_VALUE_ARRAY);
    NatObject *index = args[0];
    assert(index->type == NAT_VALUE_INTEGER); // TODO: accept a range
    assert(index->integer >= 0); // TODO: accept negative index
    if (index->integer >= self->ary_len) {
        return env_get(env, "nil");
    } else if (argc == 1) {
        return self->ary[index->integer];
    } else {
        NatObject *size = args[1];
        assert(size->type == NAT_VALUE_INTEGER);
        assert(index->integer >= 0);
        size_t end = index->integer + size->integer;
        size_t max = self->ary_len-1;
        max = end > max ? max : end;
        NatObject *result = nat_array(env);
        for (size_t i=index->integer; i<max; i++) {
            nat_array_push(result, self->ary[i]);
        }
        return result;
    }
}

NatObject *Array_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_ARRAY);
    assert(argc == 0);
    return nat_integer(env, self->ary_len);
}

NatObject *Array_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_ARRAY);
    assert(argc == 1);
    NatObject *arg = args[0];
    if (arg->type != NAT_VALUE_ARRAY) return env_get(env, "false");
    if (self->ary_len != arg->ary_len) return env_get(env, "false");
    if (self->ary_len == 0) return env_get(env, "true");
    for (size_t i=0; i<self->ary_len; i++) {
        // TODO: could easily be optimized for strings and numbers
        NatObject *result = nat_send(env, self->ary[i], "==", 1, &arg->ary[i], NULL);
        if (result->type == NAT_VALUE_FALSE) return result;
    }
    return env_get(env, "true");
}

NatObject *Array_each(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(argc == 0);
    assert(block);
    for (size_t i=0; i<self->ary_len; i++) {
        NatObject *obj = self->ary[i];
        block->fn(block->env, self, 1, &obj, NULL, NULL);
    }
    return self;
}
