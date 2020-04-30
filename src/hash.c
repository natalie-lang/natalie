#include "builtin.h"
#include "gc.h"
#include "natalie.h"

NatObject *Hash_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_MOST(1);
    NatObject *hash = nat_hash(env);
    if (block) {
        NAT_ASSERT_ARGC(0);
        hash->hash_default_block = block;
    } else if (argc == 1) {
        hash->hash_default_value = args[0];
    }
    return hash;
}

// Hash[]
NatObject *Hash_square_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    if (argc == 0) {
        return nat_hash(env);
    } else if (argc == 1) {
        NatObject *value = args[0];
        if (NAT_TYPE(value) == NAT_VALUE_HASH) {
            return value;
        } else if (NAT_TYPE(value) == NAT_VALUE_ARRAY) {
            NatObject *hash = nat_hash(env);
            for (size_t i = 0; i < value->ary_len; i++) {
                NatObject *pair = value->ary[i];
                if (NAT_TYPE(pair) != NAT_VALUE_ARRAY) {
                    NAT_RAISE(env, "ArgumentError", "wrong element in array to Hash[]");
                }
                if (pair->ary_len < 1 || pair->ary_len > 2) {
                    NAT_RAISE(env, "ArgumentError", "invalid number of elements (%d for 1..2)", pair->ary_len);
                }
                NatObject *key = pair->ary[0];
                NatObject *value = pair->ary_len == 1 ? NAT_NIL : pair->ary[1];
                nat_hash_put(env, hash, key, value);
            }
            return hash;
        }
    }
    if (argc % 2 != 0) {
        NAT_RAISE(env, "ArgumentError", "odd number of arguments for Hash");
    }
    NatObject *hash = nat_hash(env);
    for (size_t i = 0; i < argc; i += 2) {
        NatObject *key = args[i];
        NatObject *value = args[i + 1];
        nat_hash_put(env, hash, key, value);
    }
    return hash;
}

NatObject *Hash_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *out = nat_string(env, "{");
    NatHashIter *iter;
    size_t last_index = self->hashmap.num_entries - 1;
    size_t index;
    for (iter = nat_hash_iter(env, self), index = 0; iter; iter = nat_hash_iter_next(env, self, iter), index++) {
        NatObject *key_repr = nat_send(env, iter->key, "inspect", 0, NULL, NULL);
        nat_string_append(env, out, key_repr->str);
        nat_string_append(env, out, "=>");
        NatObject *val_repr = nat_send(env, iter->val, "inspect", 0, NULL, NULL);
        nat_string_append(env, out, val_repr->str);
        if (index < last_index) {
            nat_string_append(env, out, ", ");
        }
    }
    nat_string_append_char(env, out, '}');
    return out;
}

NatObject *Hash_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *key = args[0];
    NatObject *val = nat_hash_get(env, self, key);
    if (val) {
        return val;
    } else {
        return nat_hash_get_default(env, self, key);
    }
}

NatObject *Hash_refeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(2);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *key = args[0];
    NatObject *val = args[1];
    nat_hash_put(env, self, key, val);
    return val;
}

NatObject *Hash_delete(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *key = args[0];
    NatObject *val = nat_hash_delete(env, self, key);
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

NatObject *Hash_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    return nat_integer(env, self->hashmap.num_entries);
}

NatObject *Hash_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *other = args[0];
    if (NAT_TYPE(other) != NAT_VALUE_HASH) {
        return NAT_FALSE;
    }
    if (self->hashmap.num_entries != other->hashmap.num_entries) {
        return NAT_FALSE;
    }
    NatHashIter *iter;
    NatObject *other_val;
    for (iter = nat_hash_iter(env, self); iter; iter = nat_hash_iter_next(env, self, iter)) {
        other_val = nat_hash_get(env, other, iter->key);
        if (!nat_truthy(nat_send(env, iter->val, "==", 1, &other_val, NULL))) {
            return NAT_FALSE;
        }
    }
    return NAT_TRUE;
}

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, the_block, argc, args, block, hash) ({ \
    NatObject *_result = _nat_run_block_internal(env, the_block, argc, args, block);                      \
    if (nat_is_break(_result)) {                                                                          \
        nat_remove_break(_result);                                                                        \
        hash->hash_is_iterating = false;                                                                  \
        return _result;                                                                                   \
    }                                                                                                     \
    _result;                                                                                              \
})

NatObject *Hash_each(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    NatHashIter *iter;
    NatObject *block_args[2];
    for (iter = nat_hash_iter(env, self); iter; iter = nat_hash_iter_next(env, self, iter)) {
        block_args[0] = iter->key;
        block_args[1] = iter->val;
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, NULL, self);
    }
    return self;
}

NatObject *Hash_keys(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *array = nat_array(env);
    NatHashIter *iter;
    for (iter = nat_hash_iter(env, self); iter; iter = nat_hash_iter_next(env, self, iter)) {
        nat_array_push(env, array, iter->key);
    }
    return array;
}

NatObject *Hash_values(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *array = nat_array(env);
    NatHashIter *iter;
    for (iter = nat_hash_iter(env, self); iter; iter = nat_hash_iter_next(env, self, iter)) {
        nat_array_push(env, array, iter->val);
    }
    return array;
}

NatObject *Hash_sort(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NatHashIter *iter;
    NatObject *ary = nat_array(env);
    for (iter = nat_hash_iter(env, self); iter; iter = nat_hash_iter_next(env, self, iter)) {
        NatObject *pair = nat_array(env);
        nat_array_push(env, pair, iter->key);
        nat_array_push(env, pair, iter->val);
        nat_array_push(env, ary, pair);
    }
    return Array_sort(env, ary, 0, NULL, NULL);
}

NatObject *Hash_is_key(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *key = args[0];
    NatObject *val = nat_hash_get(env, self, key);
    if (val) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}
