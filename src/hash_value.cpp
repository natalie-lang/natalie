#include "natalie.hpp"

namespace Natalie {

// this is used by the hashmap library and assumes that obj->env has been set
size_t HashValue::hash(const void *key) {
    Key *key_p = (Key *)key;
    assert(NAT_OBJ_HAS_ENV2(key_p));
    assert(key_p->env.global_env);
    Value *hash_obj = key_p->key->send(&key_p->env, "hash");
    assert(NAT_TYPE(hash_obj) == Value::Type::Integer);
    return hash_obj->as_integer()->to_int64_t();
}

// this is used by the hashmap library to compare keys
int HashValue::compare(const void *a, const void *b) {
    Key *a_p = (Key *)a;
    Key *b_p = (Key *)b;
    // NOTE: Only one of the keys will have a relevant Env, i.e. the one with a non-null global_env.
    // This is a bit of a hack to get around the fact that we can't pass any extra args to hashmap_* functions.
    // TODO: Write our own hashmap implementation that passes Env around. :^)
    Env *env = a_p->env.global_env ? &a_p->env : &b_p->env;
    assert(env);
    assert(env->global_env);
    Value *a_hash = a_p->key->send(env, "hash");
    Value *b_hash = b_p->key->send(env, "hash");
    assert(NAT_TYPE(a_hash) == Value::Type::Integer);
    assert(NAT_TYPE(b_hash) == Value::Type::Integer);
    return a_hash->as_integer()->to_int64_t() - b_hash->as_integer()->to_int64_t();
}

Value *HashValue::get(Env *env, Value *key) {
    Key key_container;
    key_container.key = key;
    key_container.env = *env;
    Val *container = static_cast<Val *>(hashmap_get(&m_hashmap, &key_container));
    Value *val = container ? container->val : NULL;
    return val;
}

Value *HashValue::get_default(Env *env, Value *key) {
    if (m_default_block) {
        Value *args[2] = { this, key };
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_default_block, 2, args, NULL);
    } else {
        return m_default_value;
    }
}

void HashValue::put(Env *env, Value *key, Value *val) {
    Key key_container;
    key_container.key = key;
    key_container.env = *env;
    Val *container = static_cast<Val *>(hashmap_get(&m_hashmap, &key_container));
    if (container) {
        container->key->val = val;
        container->val = val;
    } else {
        if (m_is_iterating) {
            NAT_RAISE(env, "RuntimeError", "can't add a new key into hash during iteration");
        }
        container = static_cast<Val *>(malloc(sizeof(Val)));
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
    Val *container = static_cast<Val *>(hashmap_remove(&m_hashmap, &key_container));
    if (container) {
        key_list_remove_node(container->key);
        Value *val = container->val;
        free(container);
        return val;
    } else {
        return NULL;
    }
}

HashValue::Key *HashValue::key_list_append(Env *env, Value *key, Value *val) {
    if (m_key_list) {
        Key *first = m_key_list;
        Key *last = m_key_list->prev;
        Key *new_last = static_cast<Key *>(malloc(sizeof(Key)));
        new_last->key = key;
        new_last->val = val;
        // <first> ... <last> <new_last> -|
        // ^______________________________|
        new_last->prev = last;
        new_last->next = first;
        new_last->env = Env::new_detatched_block_env(env);
        new_last->removed = false;
        first->prev = new_last;
        last->next = new_last;
        return new_last;
    } else {
        Key *node = static_cast<Key *>(malloc(sizeof(Key)));
        node->key = key;
        node->val = val;
        node->prev = node;
        node->next = node;
        node->env = Env::new_detatched_block_env(env);
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
        node->prev = NULL;
        node->next = NULL;
        node->removed = true;
        m_key_list = NULL;
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

}
