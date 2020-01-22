#include "natalie.h"
#include "nat_array.h"

NatObject *Array_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
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
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    nat_array_push(self, arg);
    return self;
}

NatObject *Array_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
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

NatObject *Array_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(self->type == NAT_VALUE_ARRAY);
    NatObject *arg = args[0];
    assert(arg->type == NAT_VALUE_ARRAY);
    NatObject *new = nat_array(env);
    for (size_t i=0; i<self->ary_len; i++) {
        NatObject *item = self->ary[i];
        int found = 0;
        for (size_t j=0; j<arg->ary_len; j++) {
            NatObject *compare_item = arg->ary[j];
            if (nat_truthy(nat_send(env, item, "==", 1, &compare_item, NULL))) {
                found = 1;
                break;
            }
        }
        if (!found) {
            nat_array_push(new, item);
        }
    }
    return new;
}

NatObject *Array_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1, 2);
    assert(self->type == NAT_VALUE_ARRAY);
    NatObject *index = args[0];
    assert(index->type == NAT_VALUE_INTEGER); // TODO: accept a range
    assert(index->integer >= 0); // TODO: accept negative index
    if (index->integer >= self->ary_len) {
        return nat_var_get(env, "nil");
    } else if (argc == 1) {
        return self->ary[index->integer];
    } else {
        NatObject *size = args[1];
        assert(size->type == NAT_VALUE_INTEGER);
        assert(index->integer >= 0);
        size_t end = index->integer + size->integer;
        size_t max = self->ary_len;
        end = end > max ? max : end;
        NatObject *result = nat_array(env);
        for (size_t i=index->integer; i<end; i++) {
            nat_array_push(result, self->ary[i]);
        }
        return result;
    }
}

NatObject *Array_refeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(2, 3);
    assert(self->type == NAT_VALUE_ARRAY);
    NatObject *index_obj = args[0];
    assert(index_obj->type == NAT_VALUE_INTEGER); // TODO: accept a range
    assert(index_obj->integer >= 0); // TODO: accept negative index
    size_t index = index_obj->integer;
    NatObject *val;
    if (argc == 2) {
        val = args[1];
        if (index < self->ary_len) {
            self->ary[index] = val;
        } else {
            nat_array_expand_with_nil(env, self, index);
            nat_array_push(self, val);
        }
        return val;
    }
    NatObject *len_obj = args[1];
    assert(len_obj->type == NAT_VALUE_INTEGER);
    size_t length = len_obj->integer;
    assert(length >= 0);
    val = args[2];
    // PERF: inefficient for large arrays where changes are being made to only the right side
    NatObject *ary2 = nat_array(env);
    // stuff before the new entry/entries
    for (size_t i=0; i<index; i++) {
        if (i >= self->ary_len) break;
        nat_array_push(ary2, self->ary[i]);
    }
    // extra nils if needed
    nat_array_expand_with_nil(env, ary2, index);
    // the new entry/entries
    if (val->type == NAT_VALUE_ARRAY) {
        for (size_t i=0; i<val->ary_len; i++) {
            nat_array_push(ary2, val->ary[i]);
        }
    } else {
        nat_array_push(ary2, val);
    }
    // stuff after the new entry/entries
    for (size_t i=index+length; i<self->ary_len; i++) {
        nat_array_push(ary2, self->ary[i]);
    }
    // FIXME: copying like this is a possible GC bug depending on our GC implementation later
    self->ary = ary2->ary;
    self->ary_len = ary2->ary_len;
    self->ary_cap = ary2->ary_cap;
    return val;
}

NatObject *Array_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    return nat_integer(env, self->ary_len);
}

NatObject *Array_any(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    return self->ary_len > 0 ? nat_var_get(env, "true") : nat_var_get(env, "false");
}

NatObject *Array_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (arg->type != NAT_VALUE_ARRAY) return nat_var_get(env, "false");
    if (self->ary_len != arg->ary_len) return nat_var_get(env, "false");
    if (self->ary_len == 0) return nat_var_get(env, "true");
    for (size_t i=0; i<self->ary_len; i++) {
        // TODO: could easily be optimized for strings and numbers
        NatObject *result = nat_send(env, self->ary[i], "==", 1, &arg->ary[i], NULL);
        if (result->type == NAT_VALUE_FALSE) return result;
    }
    return nat_var_get(env, "true");
}

NatObject *Array_each(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(block);
    for (size_t i=0; i<self->ary_len; i++) {
        NatObject *obj = self->ary[i];
        nat_run_block(env, block, 1, &obj, NULL, NULL);
    }
    return self;
}

NatObject *Array_map(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(block);
    NatObject *new = nat_array(env);
    for (size_t i=0; i<self->ary_len; i++) {
        NatObject *item = self->ary[i];
        NatObject *result = nat_run_block(env, block, 1, &item, NULL, NULL);
        nat_array_push(new, result);
    }
    return new;
}

NatObject *Array_first(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    assert(self->type == NAT_VALUE_ARRAY);
    if (self->ary_len > 0) {
        return self->ary[0];
    } else {
        return nat_var_get(env, "nil");
    }
}

NatObject *Array_last(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    assert(self->type == NAT_VALUE_ARRAY);
    if (self->ary_len > 0) {
        return self->ary[self->ary_len - 1];
    } else {
        return nat_var_get(env, "nil");
    }
}

NatObject *Array_to_ary(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return self;
}

NatObject *Array_pop(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    if (self->ary_len == 0) {
        return nat_var_get(env, "nil");
    } else {
        NatObject *val = self->ary[self->ary_len - 1];
        self->ary_len--;
        return val;
    }
}

NatObject *Array_include(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(self->type == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(1);
    NatObject *item = args[0];
    if (self->ary_len == 0) {
        return nat_var_get(env, "false");
    } else {
        for (size_t i=0; i<self->ary_len; i++) {
            NatObject *compare_item = self->ary[i];
            if (nat_truthy(nat_send(env, item, "==", 1, &compare_item, NULL))) {
                return nat_var_get(env, "true");
            }
        }
        return nat_var_get(env, "false");
    }
}
