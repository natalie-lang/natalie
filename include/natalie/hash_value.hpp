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
        ValuePtr key { nullptr };
        ValuePtr val { nullptr };
        nat_int_t hash { 0 };
        Env env {};
        bool removed { false };
    };

    struct Val {
        Key *key { nullptr };
        ValuePtr val { nullptr };
    };

    HashValue(Env *env)
        : HashValue { env, env->Object()->const_fetch("Hash")->as_class() } { }

    HashValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Hash, klass }
        , m_default_value { env->nil_obj() } {
        hashmap_init(&m_hashmap, hash, compare, 256);
    }

    HashValue(Env *env, HashValue &other)
        : Value { other } {
        hashmap_init(&m_hashmap, hash, compare, 256);
        for (auto node : other) {
            put(env, node.key, node.val);
        }
    }

    static ValuePtr square_new(Env *, size_t argc, ValuePtr *args);

    static nat_int_t hash(const void *);
    static int compare(const void *, const void *);

    size_t size() { return m_hashmap.num_entries; }
    ValuePtr size(Env *);

    bool is_empty() { return m_hashmap.num_entries == 0; }

    ValuePtr get(Env *, ValuePtr);
    ValuePtr get_default(Env *, ValuePtr);
    void put(Env *, ValuePtr, ValuePtr);
    ValuePtr remove(Env *, ValuePtr);
    ValuePtr default_proc(Env *);
    ValuePtr default_value(Env *);

    ValuePtr default_value() { return m_default_value; }
    void set_default_value(ValuePtr val) { m_default_value = val; }

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

    ValuePtr delete_key(Env *, ValuePtr);
    ValuePtr each(Env *, Block *);
    ValuePtr eq(Env *, ValuePtr);
    ValuePtr has_key(Env *, ValuePtr);
    ValuePtr initialize(Env *, ValuePtr, Block *);
    ValuePtr inspect(Env *);
    ValuePtr keys(Env *);
    ValuePtr merge(Env *, size_t, ValuePtr *);
    ValuePtr merge_bang(Env *, size_t, ValuePtr *);
    ValuePtr ref(Env *, ValuePtr);
    ValuePtr refeq(Env *, ValuePtr, ValuePtr);
    ValuePtr sort(Env *);
    ValuePtr values(Env *);

private:
    void key_list_remove_node(Key *);
    Key *key_list_append(Env *, ValuePtr, ValuePtr);

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
    hashmap m_hashmap {};
    bool m_is_iterating { false };
    ValuePtr m_default_value { nullptr };
    Block *m_default_block { nullptr };
};
}
