#pragma once

#include <assert.h>

#include "natalie/block.hpp"
#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/nil_object.hpp"
#include "natalie/object.hpp"
#include "tm/recursion_guard.hpp"

namespace Natalie {

class HashObject : public Object {
public:
    struct Key : public Cell {
        Key *prev { nullptr };
        Key *next { nullptr };
        Value key { nullptr };
        Value val { nullptr };
        size_t hash { 0 };
        bool removed { false };

        virtual void visit_children(Visitor &visitor) override final {
            visitor.visit(prev);
            visitor.visit(next);
            visitor.visit(key);
            visitor.visit(val);
        }
    };

    HashObject()
        : HashObject { GlobalEnv::the()->Hash() } { }

    HashObject(ClassObject *klass)
        : Object { Object::Type::Hash, klass }
        , m_default_value { NilObject::the() } { }

    HashObject(Env *env, const HashObject &other)
        : Object { other }
        , m_is_comparing_by_identity { other.m_is_comparing_by_identity } {
        for (auto node : other) {
            put(env, node.key, node.val);
        }
    }

    HashObject &operator=(HashObject &&other) {
        Object::operator=(std::move(other));
        m_hashmap.clear();
        m_hashmap = std::move(other.m_hashmap);
        m_key_list = other.m_key_list;
        m_is_comparing_by_identity = other.m_is_comparing_by_identity;
        m_default_value = other.m_default_value;
        m_default_proc = other.m_default_proc;
        other.m_key_list = nullptr;
        return *this;
    }

    static Value square_new(Env *, size_t argc, Value *args, ClassObject *klass);

    static size_t hash(const void *);
    static bool compare(const void *, const void *, void *);

    size_t size() const { return m_hashmap.size(); }
    Value size(Env *) const;

    bool is_empty() { return m_hashmap.size() == 0; }

    Value get(Env *, Value);
    Value get_default(Env *, Value = nullptr);
    Value set_default(Env *, Value);

    void put(Env *, Value, Value);
    Value remove(Env *, Value);
    Value clear(Env *);

    Value default_proc(Env *);
    Value set_default_proc(Env *, Value);
    void set_default_proc(ProcObject *proc) { m_default_proc = proc; }

    Value default_value() { return m_default_value; }

    Value compact(Env *);
    Value compact_in_place(Env *);

    bool is_iterating() const { return m_is_iterating; }
    void set_is_iterating(bool is_iterating) { m_is_iterating = is_iterating; }

    class iterator {
    public:
        iterator(Key *key, const HashObject *hash)
            : m_key { key }
            , m_hash { hash } { }

        iterator operator++() {
            if (m_key->next == nullptr || (!m_key->removed && m_key->next == m_hash->m_key_list)) {
                m_key = nullptr;
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
        const HashObject *m_hash;
    };

    iterator begin() const {
        return iterator { m_key_list, this };
    }

    iterator end() const {
        return iterator { nullptr, this };
    }

    Value compare_by_identity(Env *);
    bool is_comparing_by_identity() const;
    Value delete_if(Env *, Block *);
    Value delete_key(Env *, Value, Block *);
    Value dig(Env *, size_t, Value *);
    Value each(Env *, Block *);
    bool eq(Env *, Value, SymbolObject *);
    bool eq(Env *, Value);
    bool eql(Env *, Value);
    bool gte(Env *, Value);
    bool gt(Env *, Value);
    bool lte(Env *, Value);
    bool lt(Env *, Value);
    Value except(Env *, size_t, Value *);
    Value fetch(Env *, Value, Value, Block *);
    Value fetch_values(Env *, size_t, Value *, Block *);
    Value hash(Env *);
    bool has_key(Env *, Value);
    bool has_value(Env *, Value);
    Value initialize(Env *, Value, Block *);
    Value inspect(Env *);
    Value keep_if(Env *, Block *);
    Value keys(Env *);
    Value merge(Env *, size_t, Value *, Block *);
    Value merge_in_place(Env *, size_t, Value *, Block *);
    Value ref(Env *, Value);
    Value refeq(Env *, Value, Value);
    Value slice(Env *, size_t, Value *);
    Value replace(Env *, Value);
    Value rehash(Env *);
    Value values(Env *);

    Value to_h(Env *, Block *);
    Value to_hash() { return this; }

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<HashObject %p size=%zu>", this, size());
    }

private:
    void key_list_remove_node(Key *);
    Key *key_list_append(Env *, Value, nat_int_t, Value);
    nat_int_t generate_key_hash(Env *, Value) const;

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
    TM::Hashmap<Key *, Value> m_hashmap { hash, compare, 10 }; // TODO: profile and tune this initial capacity
    bool m_is_iterating { false };
    bool m_is_comparing_by_identity { false };
    Value m_default_value { nullptr };
    ProcObject *m_default_proc { nullptr };
};
}
