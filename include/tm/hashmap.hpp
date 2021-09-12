#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace TM {

enum class HashType {
    Pointer,
    String,
};

template <typename KeyT, typename T = void *>
class Hashmap {
public:
    using HashFn = size_t(const void *);
    using CompareFn = bool(const void *, const void *, void *);

    static constexpr float HASHMAP_MIN_LOAD_FACTOR = 0.25;
    static constexpr float HASHMAP_TARGET_LOAD_FACTOR = 0.5;
    static constexpr float HASHMAP_MAX_LOAD_FACTOR = 0.75;

    struct Item {
        KeyT key;
        T value;
        Item *next { nullptr };
    };

    Hashmap(HashType hash_type = HashType::Pointer, size_t initial_capacity = 10) {
        switch (hash_type) {
        case HashType::Pointer:
            m_hash_fn = &Hashmap::hash_ptr;
            m_compare_fn = &Hashmap::compare_ptr;
            break;
        case HashType::String:
            m_hash_fn = &Hashmap::hash_str;
            m_compare_fn = &Hashmap::compare_str;
            break;
        }
        m_capacity = calculate_map_size(initial_capacity);
        m_map = new Item *[m_capacity] {};
    }

    // HashType::Pointer
    static size_t hash_ptr(const void *ptr) {
        // https://stackoverflow.com/a/12996028/197498
        auto x = (size_t)ptr;
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        x = x ^ (x >> 31);
        return x;
    }
    static bool compare_ptr(const void *a, const void *b, void *) { return a == b; }

    // HashType::String
    static size_t hash_str(const void *ptr) {
        // djb2 hash algorithm by Dan Bernstein
        auto str = static_cast<const char *>(ptr);
        size_t hash = 5381;
        int c;
        while ((c = *str++))
            hash = ((hash << 5) + hash) + c;
        return hash;
    }

    static bool compare_str(const void *a, const void *b, void *) {
        return strcmp(static_cast<const char *>(a), static_cast<const char *>(b)) == 0;
    }

    Hashmap(const Hashmap &other)
        : m_size { other.m_size }
        , m_capacity { other.m_capacity }
        , m_hash_fn { other.m_hash_fn }
        , m_compare_fn { other.m_compare_fn } {
        m_map = new Item *[m_capacity] {};
        copy_items_from(other);
    }

    Hashmap &operator=(const Hashmap &other) {
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_hash_fn = other.m_hash_fn;
        m_compare_fn = other.m_compare_fn;
        delete[] m_map;
        m_map = new Item *[m_capacity] {};
        copy_items_from(other);
        return *this;
    }

    Hashmap &operator=(Hashmap &&other) {
        delete[] m_map;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_map = other.m_map;
        m_hash_fn = other.m_hash_fn;
        m_compare_fn = other.m_compare_fn;
        other.m_size = 0;
        other.m_capacity = 0;
        other.m_map = nullptr;
        return *this;
    }

    ~Hashmap() {
        if (!m_map)
            return;
        for (size_t i = 0; i < m_capacity; i++) {
            auto item = m_map[i];
            while (item) {
                auto next_item = item->next;
                delete item;
                item = next_item;
            }
        }
        delete[] m_map;
    }

    T get(KeyT key, void *data = nullptr) {
        auto hash = m_hash_fn(key);
        auto item = find_item(key, hash, data);
        if (item)
            return item->value;
        return nullptr;
    }

    Item *find_item(KeyT key, size_t hash, void *data = nullptr) {
        assert(m_map);
        auto index = hash % m_capacity;
        auto item = m_map[index];
        while (item) {
            if (m_compare_fn(key, item->key, data))
                return item;
            item = item->next;
        }
        return nullptr;
    }

    void set(KeyT key) {
        put(key, this); // we just put a placeholder value, a pointer to this Hashmap
    }

    void put(KeyT key, T value, void *data = nullptr) {
        assert(m_map);
        if (load_factor() > HASHMAP_MAX_LOAD_FACTOR)
            rehash();
        auto hash = m_hash_fn(key);
        Item *item;
        if ((item = find_item(key, hash, data))) {
            item->value = value;
            return;
        }
        auto index = hash % m_capacity;
        auto new_item = new Item { key, value };
        insert_item(m_map, index, new_item);
        m_size++;
    }

    T remove(KeyT key, void *data = nullptr) {
        assert(m_map);
        auto hash = m_hash_fn(key);
        auto index = hash % m_capacity;
        auto item = m_map[index];
        if (item) {
            // m_map[index] = [item] -> item -> item
            //                ^ this one
            if (m_compare_fn(key, item->key, data)) {
                auto value = item->value;
                delete_item(index, item);
                return value;
            }
            auto chained_item = item->next;
            while (chained_item) {
                // m_map[index] = item -> [item] -> item
                //                        ^ this one
                if (m_compare_fn(key, chained_item->key, data)) {
                    auto value = chained_item->value;
                    delete_item(item, chained_item);
                    return value;
                }
                item = chained_item;
                chained_item = chained_item->next;
            }
        }
        return nullptr;
    }

    size_t size() const { return m_size; }
    bool is_empty() const { return m_size == 0; }

    class iterator {
    public:
        iterator(Hashmap &hashmap, size_t index, Item *item)
            : m_hashmap { hashmap }
            , m_index { index }
            , m_item { item } { }

        iterator &operator++() {
            assert(m_index < m_hashmap.m_capacity);
            if (m_item->next)
                m_item = m_item->next;
            else {
                do {
                    ++m_index;
                    if (m_index >= m_hashmap.m_capacity) {
                        m_item = nullptr;
                        return *this; // reached the end
                    }
                    m_item = m_hashmap.m_map[m_index];
                } while (!m_item);
            }
            return *this;
        }

        iterator operator++(int _) {
            iterator i = *this;
            ++(*this);
            return i;
        }

        KeyT key() {
            if (m_item)
                return m_item->key;
            return nullptr;
        }

        T value() {
            if (m_item)
                return m_item->value;
            return nullptr;
        }

        Item *operator*() {
            return m_item;
        }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_index == i2.m_index && i1.m_item == i2.m_item;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_index != i2.m_index || i1.m_item != i2.m_item;
        }

        Item *item() { return m_item; }

    private:
        Hashmap &m_hashmap;
        size_t m_index { 0 };
        Hashmap::Item *m_item { nullptr };
    };

    iterator begin() {
        assert(m_map);
        Item *item = nullptr;
        size_t index;
        for (index = 0; index < m_capacity; ++index) {
            item = m_map[index];
            if (item)
                break;
        }
        return iterator { *this, index, item };
    }

    iterator end() {
        assert(m_map);
        return iterator { *this, m_capacity, nullptr };
    }

    class MaxCapacityException { };

private:
    float load_factor() { return (float)m_size / m_capacity; }

    size_t calculate_map_size(size_t num_items) {
        for (size_t i = 4; i < 10000; ++i) {
            auto size = pow(2, i) - 1;
            if (num_items / size < HASHMAP_TARGET_LOAD_FACTOR)
                return size;
        }
        throw new MaxCapacityException;
    }

    void rehash() {
        auto new_capacity = calculate_map_size(m_size);
        auto new_map = new Item *[new_capacity] {};
        for (size_t i = 0; i < m_capacity; i++) {
            auto item = m_map[i];
            while (item) {
                auto next_item = item->next;
                auto new_hash = m_hash_fn(item->key);
                auto new_index = new_hash % new_capacity;
                insert_item(new_map, new_index, item);
                item = next_item;
            }
        }
        m_capacity = new_capacity;
        auto old_map = m_map;
        m_map = new_map;
        delete[] old_map;
    }

    void insert_item(Item **map, size_t index, Item *item) {
        auto existing_item = map[index];
        if (existing_item) {
            while (existing_item && existing_item->next) {
                existing_item = existing_item->next;
            }
            existing_item->next = item;
        } else {
            map[index] = item;
        }
        item->next = nullptr;
    }

    void delete_item(size_t index, Item *item) {
        m_map[index] = item->next;
        delete item;
        m_size--;
        if (load_factor() < HASHMAP_MIN_LOAD_FACTOR)
            rehash();
    }

    void delete_item(Item *item_before, Item *item) {
        item_before->next = item->next;
        delete item;
        m_size--;
        if (load_factor() < HASHMAP_MIN_LOAD_FACTOR)
            rehash();
    }

    void copy_items_from(const Hashmap &other) {
        for (size_t i = 0; i < m_capacity; i++) {
            auto item = other.m_map[i];
            if (item) {
                auto my_item = new Item { *item };
                m_map[i] = my_item;
                while (item->next) {
                    item = item->next;
                    my_item->next = new Item { *item };
                    my_item = my_item->next;
                }
            }
        }
    }

    size_t m_size { 0 };
    size_t m_capacity { 0 };
    Item **m_map { nullptr };

    HashFn *m_hash_fn { nullptr };
    CompareFn *m_compare_fn { nullptr };
};
}
