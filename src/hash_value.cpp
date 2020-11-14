#include "natalie.hpp"

namespace Natalie {

// this is used by the hashmap library and assumes that obj->env has been set
size_t HashValue::hash(const void *key) {
    return static_cast<const HashValue::Key *>(key)->hash;
}

// this is used by the hashmap library to compare keys
int HashValue::compare(const void *a, const void *b) {
    Key *a_p = (Key *)a;
    Key *b_p = (Key *)b;
    // NOTE: Only one of the keys will have a relevant Env, i.e. the one with a non-null global_env.
    // This is a bit of a hack to get around the fact that we can't pass any extra args to hashmap_* functions.
    // TODO: Write our own hashmap implementation that passes Env around. :^)
    Env *env = a_p->env.global_env() ? &a_p->env : &b_p->env;
    assert(env);
    assert(env->global_env());
    return a_p->key->send(env, "eql?", 1, &b_p->key)->is_truthy() ? 0 : 1; // return 0 for exact match
}

Value *HashValue::get(Env *env, Value *key) {
    Key key_container;
    key_container.key = key;
    key_container.env = *env;
    key_container.hash = key->send(env, "hash")->as_integer()->to_int64_t();
    Val *container = static_cast<Val *>(hashmap_get(&m_hashmap, &key_container));
    Value *val = container ? container->val : nullptr;
    return val;
}

Value *HashValue::get_default(Env *env, Value *key) {
    if (m_default_block) {
        Value *args[2] = { this, key };
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_default_block, 2, args, nullptr);
    } else {
        return m_default_value;
    }
}

void HashValue::put(Env *env, Value *key, Value *val) {
    this->assert_not_frozen(env);
    Key key_container;
    key_container.key = key;
    key_container.env = *env;
    key_container.hash = key->send(env, "hash")->as_integer()->to_int64_t();
    Val *container = static_cast<Val *>(hashmap_get(&m_hashmap, &key_container));
    if (container) {
        container->key->val = val;
        container->val = val;
    } else {
        if (m_is_iterating) {
            env->raise("RuntimeError", "can't add a new key into hash during iteration");
        }
        container = static_cast<Val *>(GC_MALLOC(sizeof(Val)));
        container->key = key_list_append(env, key, val);
        container->val = val;
        hashmap_put(&m_hashmap, container->key, container);
        // NOTE: env must be current and relevant at all times
        // See note on hashmap_compare for more details
        container->key->env = {};
    }
}

Value *HashValue::remove(Env *env, Value *key) {
    Key key_container;
    key_container.key = key;
    key_container.env = *env;
    key_container.hash = key->send(env, "hash")->as_integer()->to_int64_t();
    Val *container = static_cast<Val *>(hashmap_remove(&m_hashmap, &key_container));
    if (container) {
        key_list_remove_node(container->key);
        Value *val = container->val;
        GC_FREE(container);
        return val;
    } else {
        return nullptr;
    }
}

Value *HashValue::default_proc(Env *env) {
    return ProcValue::from_block_maybe(env, m_default_block);
}

Value *HashValue::default_value(Env *env) {
    if (m_default_value)
        return m_default_value;

    return env->nil_obj();
}

HashValue::Key *HashValue::key_list_append(Env *env, Value *key, Value *val) {
    if (m_key_list) {
        Key *first = m_key_list;
        Key *last = m_key_list->prev;
        Key *new_last = static_cast<Key *>(GC_MALLOC(sizeof(Key)));
        new_last->key = key;
        new_last->val = val;
        // <first> ... <last> <new_last> -|
        // ^______________________________|
        new_last->prev = last;
        new_last->next = first;
        new_last->env = Env::new_detatched_env(env);
        new_last->hash = key->send(env, "hash")->as_integer()->to_int64_t();
        new_last->removed = false;
        first->prev = new_last;
        last->next = new_last;
        return new_last;
    } else {
        Key *node = static_cast<Key *>(GC_MALLOC(sizeof(Key)));
        node->key = key;
        node->val = val;
        node->prev = node;
        node->next = node;
        node->env = Env::new_detatched_env(env);
        node->hash = key->send(env, "hash")->as_integer()->to_int64_t();
        node->removed = false;
        m_key_list = node;
        return node;
    }
}

void HashValue::key_list_remove_node(Key *node) {
    Key *prev = node->prev;
    Key *next = node->next;
    // <prev> <-> <node> <-> <next>
    if (node == next) {
        // <node> -|
        // ^_______|
        node->prev = nullptr;
        node->next = nullptr;
        node->removed = true;
        m_key_list = nullptr;
        return;
    } else if (m_key_list == node) {
        // starting point is the node to be removed, so shift them forward by one
        m_key_list = next;
    }
    // remove the node
    node->removed = true;
    prev->next = next;
    next->prev = prev;
}

Value *HashValue::initialize(Env *env, Value *default_value, Block *block) {
    if (block) {
        if (default_value) {
            env->raise("ArgumentError", "wrong number of arguments (given 1, expected 0)");
        }
        set_default_block(block);
    } else if (default_value) {
        set_default_value(default_value);
    }
    return this;
}

// Hash[]
Value *HashValue::square_new(Env *env, ssize_t argc, Value **args) {
    if (argc == 0) {
        return new HashValue { env };
    } else if (argc == 1) {
        Value *value = args[0];
        if (value->type() == Value::Type::Hash) {
            return value;
        } else if (value->type() == Value::Type::Array) {
            HashValue *hash = new HashValue { env };
            for (auto &pair : *value->as_array()) {
                if (pair->type() != Value::Type::Array) {
                    env->raise("ArgumentError", "wrong element in array to Hash[]");
                }
                ssize_t size = pair->as_array()->size();
                if (size < 1 || size > 2) {
                    env->raise("ArgumentError", "invalid number of elements (%d for 1..2)", size);
                }
                Value *key = (*pair->as_array())[0];
                Value *value = size == 1 ? env->nil_obj() : (*pair->as_array())[1];
                hash->put(env, key, value);
            }
            return hash;
        }
    }
    if (argc % 2 != 0) {
        env->raise("ArgumentError", "odd number of arguments for Hash");
    }
    HashValue *hash = new HashValue { env };
    for (ssize_t i = 0; i < argc; i += 2) {
        Value *key = args[i];
        Value *value = args[i + 1];
        hash->put(env, key, value);
    }
    return hash;
}

Value *HashValue::inspect(Env *env) {
    StringValue *out = new StringValue { env, "{" };
    ssize_t last_index = size() - 1;
    ssize_t index = 0;
    for (HashValue::Key &node : *this) {
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

Value *HashValue::ref(Env *env, Value *key) {
    Value *val = get(env, key);
    if (val) {
        return val;
    } else {
        return get_default(env, key);
    }
}

Value *HashValue::refeq(Env *env, Value *key, Value *val) {
    put(env, key, val);
    return val;
}

Value *HashValue::delete_key(Env *env, Value *key) {
    this->assert_not_frozen(env);
    Value *val = remove(env, key);
    if (val) {
        return val;
    } else {
        return env->nil_obj();
    }
}

Value *HashValue::size(Env *env) {
    assert(size() <= NAT_MAX_INT);
    return new IntegerValue { env, static_cast<int64_t>(size()) };
}

Value *HashValue::eq(Env *env, Value *other_value) {
    if (!other_value->is_hash()) {
        return env->false_obj();
    }
    HashValue *other = other_value->as_hash();
    if (size() != other->size()) {
        return env->false_obj();
    }
    Value *other_val;
    for (HashValue::Key &node : *this) {
        other_val = other->get(env, node.key);
        if (!other_val) {
            return env->false_obj();
        }
        if (!node.val->send(env, "==", 1, &other_val, nullptr)->is_truthy()) {
            return env->false_obj();
        }
    }
    return env->true_obj();
}

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, the_block, argc, args, block, hash) ({ \
    Value *_result = the_block->_run(env, argc, args, block);                                             \
    if (_result->has_break_flag()) {                                                                      \
        _result->remove_break_flag();                                                                     \
        hash->set_is_iterating(false);                                                                    \
        return _result;                                                                                   \
    }                                                                                                     \
    _result;                                                                                              \
})

Value *HashValue::each(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    Value *block_args[2];
    for (HashValue::Key &node : *this) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, nullptr, this);
    }
    return this;
}

Value *HashValue::keys(Env *env) {
    ArrayValue *array = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        array->push(node.key);
    }
    return array;
}

Value *HashValue::values(Env *env) {
    ArrayValue *array = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        array->push(node.val);
    }
    return array;
}

Value *HashValue::sort(Env *env) {
    ArrayValue *ary = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        ArrayValue *pair = new ArrayValue { env };
        pair->push(node.key);
        pair->push(node.val);
        ary->push(pair);
    }
    return ary->sort(env);
}

Value *HashValue::has_key(Env *env, Value *key) {
    Value *val = get(env, key);
    if (val) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

}
