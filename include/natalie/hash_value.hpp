#pragma once

#include <assert.h>

#include "natalie/block.hpp"
#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/nil_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct HashValue : Value {
    struct Key {
        Key *prev { nullptr };
        Key *next { nullptr };
        Value *key { nullptr };
        Value *val { nullptr };
        Env env {};
        bool removed { false };
    };

    struct Val {
        Key *key { nullptr };
        Value *val { nullptr };
    };

    HashValue(Env *env)
        : Value { env, Value::Type::Hash, NAT_OBJECT->const_get(env, "Hash", true)->as_class() }
        , m_default_value { NAT_NIL } {
        hashmap_init(&m_hashmap, hash, compare, 256);
    }

    ~HashValue() {
        /*
        destroy_key_list();
        for (HashValue::Key &node : *this) {
            delete static_cast<Val *>(hashmap_get(&m_hashmap, &node));
        }
        hashmap_destroy(&m_hashmap);
        delete m_default_block;
        */
    }

    static size_t hash(const void *);
    static int compare(const void *, const void *);

    ssize_t size() { return m_hashmap.num_entries; }
    Value *get(Env *, Value *);
    Value *get_default(Env *, Value *);
    void put(Env *, Value *, Value *);
    Value *remove(Env *, Value *);

    const Value *default_value() { return m_default_value; }
    void set_default_value(Value *val) { m_default_value = val; }

    const Block *default_block() { return m_default_block; }
    void set_default_block(Block *block) { m_default_block = block; }

    bool is_iterating() { return m_is_iterating; }
    void set_is_iterating(bool is_iterating) { m_is_iterating = is_iterating; }

    class iterator {
    public:
        iterator(Key *key, HashValue *hash)
            : m_key { key }
            , m_hash { hash } {
            if (m_key) m_hash->set_is_iterating(true);
        }

        iterator operator++() {
            if (m_key->next == nullptr || (!m_key->removed && m_key->next == m_hash->m_key_list)) {
                m_key = nullptr;
                m_hash->set_is_iterating(false);
            } else if (m_key->next->removed) {
                m_key = m_key->next;
                return operator++();
            } else {
                m_key = m_key->next;
            }
            return *this;
        }

        iterator operator++(int _) {
            iterator i = *this;
            operator++();
            return i;
        }

        Key &operator*() { return *m_key; }
        Key *operator->() { return m_key; }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_key == i2.m_key;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_key != i2.m_key;
        }

    private:
        Key *m_key;
        HashValue *m_hash;
    };

    iterator begin() {
        return iterator { m_key_list, this };
    }

    iterator end() {
        return iterator { nullptr, this };
    }

private:
    void key_list_remove_node(Key *);
    Key *key_list_append(Env *, Value *, Value *);

    void destroy_key_list() {
        if (!m_key_list) return;
        Key *first_key = m_key_list;
        Key *key = m_key_list;
        do {
            Key *next_key = key->next;
            delete key;
            key = next_key;
        } while (key != first_key);
    }

    Key *m_key_list { nullptr };
    struct hashmap m_hashmap EMPTY_HASHMAP;
    bool m_is_iterating { false };
    Value *m_default_value { nullptr };
    Block *m_default_block { nullptr };
};
}
