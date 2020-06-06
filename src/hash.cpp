#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Hash_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    HashValue *hash = hash_new(env);
    if (block) {
        NAT_ASSERT_ARGC(0);
        hash->hash_default_block = block;
    } else if (argc == 1) {
        hash->hash_default_value = args[0];
    }
    return hash;
}

// Hash[]
Value *Hash_square_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    if (argc == 0) {
        return hash_new(env);
    } else if (argc == 1) {
        Value *value = args[0];
        if (NAT_TYPE(value) == Value::Type::Hash) {
            return value;
        } else if (NAT_TYPE(value) == Value::Type::Array) {
            HashValue *hash = hash_new(env);
            for (ssize_t i = 0; i < vector_size(&value->as_array()->ary); i++) {
                Value *pair = static_cast<Value *>(vector_get(&value->as_array()->ary, i));
                if (NAT_TYPE(pair) != Value::Type::Array) {
                    NAT_RAISE(env, "ArgumentError", "wrong element in array to Hash[]");
                }
                if (vector_size(&pair->as_array()->ary) < 1 || vector_size(&pair->as_array()->ary) > 2) {
                    NAT_RAISE(env, "ArgumentError", "invalid number of elements (%d for 1..2)", vector_size(&pair->as_array()->ary));
                }
                Value *key = static_cast<Value *>(vector_get(&pair->as_array()->ary, 0));
                Value *value = vector_size(&pair->as_array()->ary) == 1 ? NAT_NIL : static_cast<Value *>(vector_get(&pair->as_array()->ary, 1));
                hash_put(env, hash, key, value);
            }
            return hash;
        }
    }
    if (argc % 2 != 0) {
        NAT_RAISE(env, "ArgumentError", "odd number of arguments for Hash");
    }
    Value *hash = hash_new(env);
    for (ssize_t i = 0; i < argc; i += 2) {
        Value *key = args[i];
        Value *value = args[i + 1];
        hash_put(env, hash, key, value);
    }
    return hash;
}

Value *Hash_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    StringValue *out = string(env, "{");
    HashIter *iter;
    ssize_t last_index = self->hashmap.num_entries - 1;
    ssize_t index;
    for (iter = hash_iter(env, self), index = 0; iter; iter = hash_iter_next(env, self, iter), index++) {
        StringValue *key_repr = send(env, iter->key, "inspect", 0, NULL, NULL)->as_string();
        string_append(env, out, key_repr->str);
        string_append(env, out, "=>");
        StringValue *val_repr = send(env, iter->val, "inspect", 0, NULL, NULL)->as_string();
        string_append(env, out, val_repr->str);
        if (index < last_index) {
            string_append(env, out, ", ");
        }
    }
    string_append_char(env, out, '}');
    return out;
}

Value *Hash_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    HashValue *self = self_value->as_hash();
    Value *key = args[0];
    Value *val = hash_get(env, self, key);
    if (val) {
        return val;
    } else {
        return hash_get_default(env, self, key);
    }
}

Value *Hash_refeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    HashValue *self = self_value->as_hash();
    NAT_ASSERT_NOT_FROZEN(self);
    Value *key = args[0];
    Value *val = args[1];
    hash_put(env, self, key, val);
    return val;
}

Value *Hash_delete(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    HashValue *self = self_value->as_hash();
    NAT_ASSERT_NOT_FROZEN(self);
    Value *key = args[0];
    Value *val = hash_delete(env, self, key);
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

Value *Hash_size(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    return integer(env, self->hashmap.num_entries);
}

Value *Hash_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    HashValue *self = self_value->as_hash();
    Value *other_value = args[0];
    if (NAT_TYPE(other_value) != Value::Type::Hash) {
        return NAT_FALSE;
    }
    HashValue *other = other_value->as_hash();
    if (self->hashmap.num_entries != other->hashmap.num_entries) {
        return NAT_FALSE;
    }
    HashIter *iter;
    Value *other_val;
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        other_val = hash_get(env, other, iter->key);
        if (!send(env, iter->val, "==", 1, &other_val, nullptr)->is_truthy()) {
            return NAT_FALSE;
        }
    }
    return NAT_TRUE;
}

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, the_block, argc, args, block, hash) ({ \
    Value *_result = _run_block_internal(env, the_block, argc, args, block);                              \
    if (is_break(_result)) {                                                                              \
        remove_break(_result);                                                                            \
        hash->hash_is_iterating = false;                                                                  \
        return _result;                                                                                   \
    }                                                                                                     \
    _result;                                                                                              \
})

Value *Hash_each(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    HashIter *iter;
    Value *block_args[2];
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        block_args[0] = iter->key;
        block_args[1] = iter->val;
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, NULL, self);
    }
    return self;
}

Value *Hash_keys(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    Value *array = array_new(env);
    HashIter *iter;
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        array_push(env, array, iter->key);
    }
    return array;
}

Value *Hash_values(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    Value *array = array_new(env);
    HashIter *iter;
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        array_push(env, array, iter->val);
    }
    return array;
}

Value *Hash_sort(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    HashIter *iter;
    HashValue *self = self_value->as_hash();
    Value *ary = array_new(env);
    for (iter = hash_iter(env, self); iter; iter = hash_iter_next(env, self, iter)) {
        Value *pair = array_new(env);
        array_push(env, pair, iter->key);
        array_push(env, pair, iter->val);
        array_push(env, ary, pair);
    }
    return Array_sort(env, ary, 0, NULL, NULL);
}

Value *Hash_is_key(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    HashValue *self = self_value->as_hash();
    assert(NAT_TYPE(self) == Value::Type::Hash);
    Value *key = args[0];
    Value *val = hash_get(env, self, key);
    if (val) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
