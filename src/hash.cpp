#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Hash_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    HashValue *hash = new HashValue { env };
    if (block) {
        NAT_ASSERT_ARGC(0);
        hash->set_default_block(block);
    } else if (argc == 1) {
        hash->set_default_value(args[0]);
    }
    return hash;
}

// Hash[]
Value *Hash_square_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    if (argc == 0) {
        return new HashValue { env };
    } else if (argc == 1) {
        Value *value = args[0];
        if (NAT_TYPE(value) == Value::Type::Hash) {
            return value;
        } else if (NAT_TYPE(value) == Value::Type::Array) {
            HashValue *hash = new HashValue { env };
            for (auto &pair : *value->as_array()) {
                if (NAT_TYPE(pair) != Value::Type::Array) {
                    NAT_RAISE(env, "ArgumentError", "wrong element in array to Hash[]");
                }
                ssize_t size = pair->as_array()->size();
                if (size < 1 || size > 2) {
                    NAT_RAISE(env, "ArgumentError", "invalid number of elements (%d for 1..2)", size);
                }
                Value *key = (*pair->as_array())[0];
                Value *value = size == 1 ? NAT_NIL : (*pair->as_array())[1];
                hash->put(env, key, value);
            }
            return hash;
        }
    }
    if (argc % 2 != 0) {
        NAT_RAISE(env, "ArgumentError", "odd number of arguments for Hash");
    }
    HashValue *hash = new HashValue { env };
    for (ssize_t i = 0; i < argc; i += 2) {
        Value *key = args[i];
        Value *value = args[i + 1];
        hash->put(env, key, value);
    }
    return hash;
}

Value *Hash_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    StringValue *out = new StringValue { env, "{" };
    ssize_t last_index = self->size() - 1;
    ssize_t index = 0;
    for (HashValue::Key &node : *self) {
        StringValue *key_repr = node.key->send(env, "inspect")->as_string();
        out->append_string(env, key_repr);
        out->append(env, "=>");
        StringValue *val_repr = node.val->send(env, "inspect")->as_string();
        out->append_string(env, val_repr);
        if (index < last_index) {
            out->append(env, ", ");
        }
        index++;
    }
    out->append_char(env, '}');
    return out;
}

Value *Hash_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    HashValue *self = self_value->as_hash();
    Value *key = args[0];
    Value *val = self->get(env, key);
    if (val) {
        return val;
    } else {
        return self->get_default(env, key);
    }
}

Value *Hash_refeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    HashValue *self = self_value->as_hash();
    NAT_ASSERT_NOT_FROZEN(self);
    Value *key = args[0];
    Value *val = args[1];
    self->put(env, key, val);
    return val;
}

Value *Hash_delete(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    HashValue *self = self_value->as_hash();
    NAT_ASSERT_NOT_FROZEN(self);
    Value *key = args[0];
    Value *val = self->remove(env, key);
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

Value *Hash_size(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    assert(self->size() <= NAT_MAX_INT);
    return new IntegerValue { env, static_cast<int64_t>(self->size()) };
}

Value *Hash_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    HashValue *self = self_value->as_hash();
    Value *other_value = args[0];
    if (NAT_TYPE(other_value) != Value::Type::Hash) {
        return NAT_FALSE;
    }
    HashValue *other = other_value->as_hash();
    if (self->size() != other->size()) {
        return NAT_FALSE;
    }
    Value *other_val;
    for (HashValue::Key &node : *self) {
        other_val = other->get(env, node.key);
        if (!node.val->send(env, "==", 1, &other_val, nullptr)->is_truthy()) {
            return NAT_FALSE;
        }
    }
    return NAT_TRUE;
}

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, the_block, argc, args, block, hash) ({ \
    Value *_result = _run_block_internal(env, the_block, argc, args, block);                              \
    if (is_break(_result)) {                                                                              \
        remove_break(_result);                                                                            \
        hash->set_is_iterating(false);                                                                    \
        return _result;                                                                                   \
    }                                                                                                     \
    _result;                                                                                              \
})

Value *Hash_each(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    Value *block_args[2];
    for (HashValue::Key &node : *self) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, nullptr, self);
    }
    return self;
}

Value *Hash_keys(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    ArrayValue *array = new ArrayValue { env };
    for (HashValue::Key &node : *self) {
        array->push(node.key);
    }
    return array;
}

Value *Hash_values(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    HashValue *self = self_value->as_hash();
    ArrayValue *array = new ArrayValue { env };
    for (HashValue::Key &node : *self) {
        array->push(node.val);
    }
    return array;
}

Value *Hash_sort(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    HashValue *self = self_value->as_hash();
    ArrayValue *ary = new ArrayValue { env };
    for (HashValue::Key &node : *self) {
        ArrayValue *pair = new ArrayValue { env };
        pair->push(node.key);
        pair->push(node.val);
        ary->push(pair);
    }
    return Array_sort(env, ary, 0, nullptr, nullptr);
}

Value *Hash_is_key(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    HashValue *self = self_value->as_hash();
    assert(NAT_TYPE(self) == Value::Type::Hash);
    Value *key = args[0];
    Value *val = self->get(env, key);
    if (val) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
