#include "natalie.hpp"

namespace Natalie {

// this is used by the hashmap library and assumes that obj->env has been set
nat_int_t HashValue::hash(const void *key) {
    return static_cast<const HashValue::Key *>(key)->hash;
}

// this is used by the hashmap library to compare keys
int HashValue::compare(Env *env, const void *a, const void *b) {
    Key *a_p = (Key *)a;
    Key *b_p = (Key *)b;
    return a_p->key.send(env, "eql?", 1, &b_p->key)->is_truthy() ? 0 : 1; // return 0 for exact match
}

ValuePtr HashValue::get(Env *env, ValuePtr key) {
    Key key_container;
    key_container.key = key;
    key_container.hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
    Val *container = m_hashmap.get(env, &key_container);
    ValuePtr val = container ? container->val : nullptr;
    return val;
}

ValuePtr HashValue::get_default(Env *env, ValuePtr key) {
    if (m_default_block) {
        ValuePtr args[2] = { this, key };
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_default_block, 2, args, nullptr);
    } else {
        return m_default_value;
    }
}

void HashValue::put(Env *env, ValuePtr key, ValuePtr val) {
    this->assert_not_frozen(env);
    Key key_container;
    key_container.key = key;
    key_container.hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
    Val *container = m_hashmap.get(env, &key_container);
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
        m_hashmap.put(env, container->key, container);
    }
}

ValuePtr HashValue::remove(Env *env, ValuePtr key) {
    Key key_container;
    key_container.key = key;
    key_container.hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
    Val *container = m_hashmap.remove(env, &key_container);
    if (container) {
        key_list_remove_node(container->key);
        ValuePtr val = container->val;
        GC_FREE(container);
        return val;
    } else {
        return nullptr;
    }
}

ValuePtr HashValue::default_proc(Env *env) {
    return ProcValue::from_block_maybe(env, m_default_block);
}

ValuePtr HashValue::default_value(Env *env) {
    if (m_default_value)
        return m_default_value;

    return env->nil_obj();
}

HashValue::Key *HashValue::key_list_append(Env *env, ValuePtr key, ValuePtr val) {
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
        new_last->hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
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
        node->hash = key.send(env, "hash")->as_integer()->to_nat_int_t();
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

ValuePtr HashValue::initialize(Env *env, ValuePtr default_value, Block *block) {
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
ValuePtr HashValue::square_new(Env *env, size_t argc, ValuePtr *args) {
    if (argc == 0) {
        return new HashValue { env };
    } else if (argc == 1) {
        ValuePtr value = args[0];
        if (value->type() == Value::Type::Hash) {
            return value;
        } else if (value->type() == Value::Type::Array) {
            HashValue *hash = new HashValue { env };
            for (auto &pair : *value->as_array()) {
                if (pair->type() != Value::Type::Array) {
                    env->raise("ArgumentError", "wrong element in array to Hash[]");
                }
                size_t size = pair->as_array()->size();
                if (size < 1 || size > 2) {
                    env->raise("ArgumentError", "invalid number of elements (%d for 1..2)", size);
                }
                ValuePtr key = (*pair->as_array())[0];
                ValuePtr value = size == 1 ? env->nil_obj() : (*pair->as_array())[1];
                hash->put(env, key, value);
            }
            return hash;
        }
    }
    if (argc % 2 != 0) {
        env->raise("ArgumentError", "odd number of arguments for Hash");
    }
    HashValue *hash = new HashValue { env };
    for (size_t i = 0; i < argc; i += 2) {
        ValuePtr key = args[i];
        ValuePtr value = args[i + 1];
        hash->put(env, key, value);
    }
    return hash;
}

ValuePtr HashValue::inspect(Env *env) {
    StringValue *out = new StringValue { env, "{" };
    size_t last_index = size() - 1;
    size_t index = 0;
    for (HashValue::Key &node : *this) {
        StringValue *key_repr = node.key.send(env, "inspect")->as_string();
        out->append_string(env, key_repr);
        out->append(env, "=>");
        StringValue *val_repr = node.val.send(env, "inspect")->as_string();
        out->append_string(env, val_repr);
        if (index < last_index) {
            out->append(env, ", ");
        }
        index++;
    }
    out->append_char(env, '}');
    return out;
}

ValuePtr HashValue::ref(Env *env, ValuePtr key) {
    ValuePtr val = get(env, key);
    if (val) {
        return val;
    } else {
        return get_default(env, key);
    }
}

ValuePtr HashValue::refeq(Env *env, ValuePtr key, ValuePtr val) {
    put(env, key, val);
    return val;
}

ValuePtr HashValue::delete_key(Env *env, ValuePtr key) {
    this->assert_not_frozen(env);
    ValuePtr val = remove(env, key);
    if (val) {
        return val;
    } else {
        return env->nil_obj();
    }
}

ValuePtr HashValue::size(Env *env) {
    return IntegerValue::from_size_t(env, size());
}

ValuePtr HashValue::eq(Env *env, ValuePtr other_value) {
    if (!other_value->is_hash()) {
        return env->false_obj();
    }
    HashValue *other = other_value->as_hash();
    if (size() != other->size()) {
        return env->false_obj();
    }
    ValuePtr other_val;
    for (HashValue::Key &node : *this) {
        other_val = other->get(env, node.key);
        if (!other_val) {
            return env->false_obj();
        }
        if (!node.val.send(env, "==", 1, &other_val, nullptr)->is_truthy()) {
            return env->false_obj();
        }
    }
    return env->true_obj();
}

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, the_block, argc, args, block, hash) ({ \
    ValuePtr _result = the_block->_run(env, argc, args, block);                                           \
    if (_result->has_break_flag()) {                                                                      \
        _result->remove_break_flag();                                                                     \
        hash->set_is_iterating(false);                                                                    \
        return _result;                                                                                   \
    }                                                                                                     \
    _result;                                                                                              \
})

ValuePtr HashValue::each(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    ValuePtr block_args[2];
    for (HashValue::Key &node : *this) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, nullptr, this);
    }
    return this;
}

ValuePtr HashValue::keys(Env *env) {
    ArrayValue *array = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        array->push(node.key);
    }
    return array;
}

ValuePtr HashValue::values(Env *env) {
    ArrayValue *array = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        array->push(node.val);
    }
    return array;
}

ValuePtr HashValue::sort(Env *env) {
    ArrayValue *ary = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        ArrayValue *pair = new ArrayValue { env };
        pair->push(node.key);
        pair->push(node.val);
        ary->push(pair);
    }
    return ary->sort(env);
}

ValuePtr HashValue::has_key(Env *env, ValuePtr key) {
    ValuePtr val = get(env, key);
    if (val) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

ValuePtr HashValue::merge(Env *env, size_t argc, ValuePtr *args) {
    return dup(env)->as_hash()->merge_bang(env, argc, args);
}

ValuePtr HashValue::merge_bang(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        auto h = args[i];
        h->assert_type(env, Value::Type::Hash, "Hash");
        for (auto node : *h->as_hash()) {
            put(env, node.key, node.val);
        }
    }
    return this;
}

}
