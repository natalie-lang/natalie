#include "natalie.h"
#include "builtin.h"

NatObject *Array_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
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
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *arg = args[0];
    nat_array_push(self, arg);
    return self;
}

NatObject *Array_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_ARRAY, "Array");
    NatObject *new = nat_array(env);
    nat_grow_array_at_least(new, self->ary_len + arg->ary_len);
    memcpy(new->ary, self->ary, self->ary_len * sizeof(NatObject*));
    memcpy(&new->ary[self->ary_len], arg->ary, arg->ary_len * sizeof(NatObject*));
    new->ary_len = self->ary_len + arg->ary_len;
    return new;
}

NatObject *Array_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_ARRAY, "Array");
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
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *index = args[0];
    NAT_ASSERT_TYPE(index, NAT_VALUE_INTEGER, "Integer"); // TODO: accept a range
    assert(NAT_INT_VALUE(index) >= 0); // TODO: accept negative index
    if (NAT_INT_VALUE(index) >= (long long)self->ary_len) {
        return NAT_NIL;
    } else if (argc == 1) {
        return self->ary[NAT_INT_VALUE(index)];
    } else {
        NatObject *size = args[1];
        NAT_ASSERT_TYPE(size, NAT_VALUE_INTEGER, "Integer");
        assert(NAT_INT_VALUE(index) >= 0);
        size_t end = NAT_INT_VALUE(index) + NAT_INT_VALUE(size);
        size_t max = self->ary_len;
        end = end > max ? max : end;
        NatObject *result = nat_array(env);
        for (size_t i=NAT_INT_VALUE(index); i<end; i++) {
            nat_array_push(result, self->ary[i]);
        }
        return result;
    }
}

NatObject *Array_refeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(2, 3);
    NAT_ASSERT_NOT_FROZEN(self);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *index_obj = args[0];
    NAT_ASSERT_TYPE(index_obj, NAT_VALUE_INTEGER, "Integer"); // TODO: accept a range
    assert(NAT_INT_VALUE(index_obj) >= 0); // TODO: accept negative index
    size_t index = NAT_INT_VALUE(index_obj);
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
    NAT_ASSERT_TYPE(len_obj, NAT_VALUE_INTEGER, "Integer");
    int length = NAT_INT_VALUE(len_obj);
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
    if (NAT_TYPE(val) == NAT_VALUE_ARRAY) {
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
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    return nat_integer(env, self->ary_len);
}

NatObject *Array_any(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    return self->ary_len > 0 ? NAT_TRUE : NAT_FALSE;
}

NatObject *Array_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) != NAT_VALUE_ARRAY) return NAT_FALSE;
    if (self->ary_len != arg->ary_len) return NAT_FALSE;
    if (self->ary_len == 0) return NAT_TRUE;
    for (size_t i=0; i<self->ary_len; i++) {
        // TODO: could easily be optimized for strings and numbers
        NatObject *result = nat_send(env, self->ary[i], "==", 1, &arg->ary[i], NULL);
        if (NAT_TYPE(result) == NAT_VALUE_FALSE) return result;
    }
    return NAT_TRUE;
}

NatObject *Array_each(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    for (size_t i=0; i<self->ary_len; i++) {
        NatObject *obj = self->ary[i];
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, NULL, NULL);
    }
    return self;
}

NatObject *Array_map(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    NatObject *new = nat_array(env);
    for (size_t i=0; i<self->ary_len; i++) {
        NatObject *item = self->ary[i];
        NatObject *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, NULL, NULL);
        nat_array_push(new, result);
    }
    return new;
}

NatObject *Array_first(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    if (self->ary_len > 0) {
        return self->ary[0];
    } else {
        return NAT_NIL;
    }
}

NatObject *Array_last(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    if (self->ary_len > 0) {
        return self->ary[self->ary_len - 1];
    } else {
        return NAT_NIL;
    }
}

NatObject *Array_to_ary(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return self;
}

NatObject *Array_pop(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_NOT_FROZEN(self);
    if (self->ary_len == 0) {
        return NAT_NIL;
    } else {
        NatObject *val = self->ary[self->ary_len - 1];
        self->ary_len--;
        return val;
    }
}

NatObject *Array_include(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(1);
    NatObject *item = args[0];
    if (self->ary_len == 0) {
        return NAT_FALSE;
    } else {
        for (size_t i=0; i<self->ary_len; i++) {
            NatObject *compare_item = self->ary[i];
            if (nat_truthy(nat_send(env, item, "==", 1, &compare_item, NULL))) {
                return NAT_TRUE;
            }
        }
        return NAT_FALSE;
    }
}

NatObject *Array_sort(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    NatObject *copy = nat_array_copy(env, self);
	nat_quicksort(env, copy->ary, 0, copy->ary_len - 1);
    return copy;
}
