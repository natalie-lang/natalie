#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Hash_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    NatObject *hash = hash_new(env);
    if (block) {
        NAT_ASSERT_ARGC(0);
        hash->hash_default_block = block;
    } else if (argc == 1) {
        hash->hash_default_value = args[0];
    }
    return hash;
}

// Hash[]
NatObject *Hash_square_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    if (argc == 0) {
        return hash_new(env);
    } else if (argc == 1) {
        NatObject *value = args[0];
        if (NAT_TYPE(value) == NAT_VALUE_HASH) {
            return value;
        } else if (NAT_TYPE(value) == NAT_VALUE_ARRAY) {
            NatObject *hash = hash_new(env);
            for (ssize_t i = 0; i < vector_size(&value->ary); i++) {
                NatObject *pair = static_cast<NatObject *>(vector_get(&value->ary, i));
                if (NAT_TYPE(pair) != NAT_VALUE_ARRAY) {
                    NAT_RAISE(env, "ArgumentError", "wrong element in array to Hash[]");
                }
                if (vector_size(&pair->ary) < 1 || vector_size(&pair->ary) > 2) {
                    NAT_RAISE(env, "ArgumentError", "invalid number of elements (%d for 1..2)", vector_size(&pair->ary));
                }
                NatObject *key = static_cast<NatObject *>(vector_get(&pair->ary, 0));
                NatObject *value = vector_size(&pair->ary) == 1 ? NAT_NIL : static_cast<NatObject *>(vector_get(&pair->ary, 1));
                hash_put(env, hash, key, value);
            }
            return hash;
        }
    }
    if (argc % 2 != 0) {
        NAT_RAISE(env, "ArgumentError", "odd number of arguments for Hash");
    }
    NatObject *hash = hash_new(env);
    for (ssize_t i = 0; i < argc; i += 2) {
        NatObject *key = args[i];
        NatObject *value = args[i + 1];
        hash_put(env, hash, key, value);
    }
    return hash;
}

NatObject *Hash_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *out = string(env, "{");
    HashIter *iter;
    ssize_t last_index = self->hashmap.num_entries - 1;
    ssize_t index;
    for (iter = hash_iter(env, self), index = 0; iter; iter = hash_iter_next(env, self, iter), index++) {
        NatObject *key_repr = send(env, iter->key, "inspect", 0, NULL, NULL);
        string_append(env, out, key_repr->str);
        string_append(env, out, "=>");
        NatObject *val_repr = send(env, iter->val, "inspect", 0, NULL, NULL);
        string_append(env, out, val_repr->str);
        if (index < last_index) {
            string_append(env, out, ", ");
        }
    }
    string_append_char(env, out, '}');
    return out;
}

NatObject *Hash_ref(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *key = args[0];
    NatObject *val = hash_get(env, self, key);
    if (val) {
        return val;
    } else {
        return hash_get_default(env, self, key);
    }
}

NatObject *Hash_refeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *key = args[0];
    NatObject *val = args[1];
    hash_put(env, self, key, val);
    return val;
}

NatObject *Hash_delete(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *key = args[0];
    NatObject *val = hash_delete(env, self, key);
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

NatObject *Hash_size(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    return integer(env, self->hashmap.num_entries);
}

NatObject *Hash_eqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *other = args[0];
    if (NAT_TYPE(other) != NAT_VALUE_HASH) {
        return NAT_FALSE;
    }
    if (self->hashmap.num_entries != other->hashmap.num_entries) {
        return NAT_FALSE;
    }
    HashIter *iter;
    NatObject *other_val;
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        other_val = hash_get(env, other, iter->key);
        if (!truthy(send(env, iter->val, "==", 1, &other_val, NULL))) {
            return NAT_FALSE;
        }
    }
    return NAT_TRUE;
}

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, the_block, argc, args, block, hash) ({ \
    NatObject *_result = _run_block_internal(env, the_block, argc, args, block);                          \
    if (is_break(_result)) {                                                                              \
        remove_break(_result);                                                                            \
        hash->hash_is_iterating = false;                                                                  \
        return _result;                                                                                   \
    }                                                                                                     \
    _result;                                                                                              \
})

NatObject *Hash_each(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    HashIter *iter;
    NatObject *block_args[2];
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        block_args[0] = iter->key;
        block_args[1] = iter->val;
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, NULL, self);
    }
    return self;
}

NatObject *Hash_keys(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *array = array_new(env);
    HashIter *iter;
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        array_push(env, array, iter->key);
    }
    return array;
}

NatObject *Hash_values(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *array = array_new(env);
    HashIter *iter;
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        array_push(env, array, iter->val);
    }
    return array;
}

NatObject *Hash_sort(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    HashIter *iter;
    NatObject *ary = array_new(env);
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        NatObject *pair = array_new(env);
        array_push(env, pair, iter->key);
        array_push(env, pair, iter->val);
        array_push(env, ary, pair);
    }
    return Array_sort(env, ary, 0, NULL, NULL);
}

NatObject *Hash_is_key(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_HASH);
    NatObject *key = args[0];
    NatObject *val = hash_get(env, self, key);
    if (val) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
