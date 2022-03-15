#pragma once

#include <algorithm>
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tm/macros.hpp"
#include "tm/string.hpp"

namespace TM {

enum class HashType {
    Pointer,
    String,
    TMString,
};

struct HashmapUtils {
    static size_t hashmap_hash_ptr(uintptr_t ptr) {
        // https://stackoverflow.com/a/12996028/197498
        auto x = (size_t)ptr;
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        x = x ^ (x >> 31);
        return x;
    }
};

template <typename KeyT, typename T = void *>
class Hashmap {
public:
    using HashFn = size_t(KeyT &);
    using CompareFn = bool(KeyT &, KeyT &, void *);

    static constexpr size_t HASHMAP_MIN_LOAD_FACTOR = 25;
    static constexpr size_t HASHMAP_TARGET_LOAD_FACTOR = 50;
    static constexpr size_t HASHMAP_MAX_LOAD_FACTOR = 75;

    struct Item {
        KeyT key;
        T value;
        size_t hash;
        Item *next { nullptr };
    };

    Hashmap(HashFn hash_fn, CompareFn compare_fn, size_t initial_capacity = 10)
        : m_capacity { calculate_map_size(initial_capacity) }
        , m_hash_fn { hash_fn }
        , m_compare_fn { compare_fn } { }

    Hashmap(HashType hash_type = HashType::Pointer, size_t initial_capacity = 10)
        : m_capacity { calculate_map_size(initial_capacity) } {
        switch (hash_type) {
        case HashType::Pointer:
            m_hash_fn = &Hashmap::hash_ptr;
            m_compare_fn = &Hashmap::compare_ptr;
            break;
        case HashType::String:
            m_hash_fn = &Hashmap::hash_str;
            m_compare_fn = &Hashmap::compare_str;
            break;
        case HashType::TMString:
            m_hash_fn = &Hashmap::hash_tm_str;
            m_compare_fn = &Hashmap::compare_tm_str;
            break;
        default:
            TM_UNREACHABLE();
        }
    }

    // HashType::Pointer
    static size_t hash_ptr(KeyT &ptr) {
        if constexpr (std::is_pointer_v<KeyT>)
            return HashmapUtils::hashmap_hash_ptr((uintptr_t)ptr);
        else
            return 0;
    }
    static bool compare_ptr(KeyT &a, KeyT &b, void *) {
        return a == b;
    }

    // HashType::String
    static size_t hash_str(KeyT &ptr) {
        if constexpr (std::is_pointer_v<KeyT>) {
            auto str = String((const char *)(ptr));
            return str.djb2_hash();
        } else {
            return 0;
        }
    }
    static bool compare_str(KeyT &a, KeyT &b, void *) {
        if constexpr (std::is_pointer_v<KeyT>)
            return strcmp((const char *)(a), (const char *)(b)) == 0;
        else
            return false;
    }

    // HashType::TMString
    static size_t hash_tm_str(KeyT &ptr) {
        if constexpr (std::is_pointer_v<KeyT>) {
            return 0;
        } else {
            auto str = (const String &)(ptr);
            return str.djb2_hash();
        }
    }
    static bool compare_tm_str(KeyT &a, KeyT &b, void *) {
        if constexpr (std::is_pointer_v<KeyT>) {
            return false;
        } else {
            return ((const String &)a) == ((const String &)b);
        }
    }

    Hashmap(const Hashmap &other)
        : m_capacity { other.m_capacity }
        , m_hash_fn { other.m_hash_fn }
        , m_compare_fn { other.m_compare_fn } {
        m_map = new Item *[m_capacity] {};
        copy_items_from(other);
    }

    Hashmap &operator=(const Hashmap &other) {
        m_capacity = other.m_capacity;
        m_hash_fn = other.m_hash_fn;
        m_compare_fn = other.m_compare_fn;
        if (m_map) {
            clear();
            delete[] m_map;
        }
        m_map = new Item *[m_capacity] {};
        copy_items_from(other);
        return *this;
    }

    Hashmap &operator=(Hashmap &&other) {
        if (m_map) {
            clear();
            delete[] m_map;
        }
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
        if (!m_map) return;
        clear();
        delete[] m_map;
    }

    T get(KeyT key, void *data = nullptr) const {
        auto hash = m_hash_fn(key);
        auto item = find_item(key, hash, data);
        if (item)
            return item->value;
        if constexpr (std::is_pointer<T>::value)
            return nullptr;
        else
            return {};
    }

    Item *find_item(KeyT key, size_t hash, void *data = nullptr) const {
        if (m_size == 0) return nullptr;
        assert(m_map);
        auto index = index_for_hash(hash);
        auto item = m_map[index];
        while (item) {
            if (hash == item->hash && m_compare_fn(key, item->key, data))
                return item;
            item = item->next;
        }
        return nullptr;
    }

    void set(KeyT key) {
        put(key, this); // we just put a placeholder value, a pointer to this Hashmap
    }

    void put(KeyT key, T value, void *data = nullptr) {
        if (!m_map)
            m_map = new Item *[m_capacity] {};
        if (load_factor() > HASHMAP_MAX_LOAD_FACTOR)
            rehash();
        auto hash = m_hash_fn(key);
        Item *item;
        if ((item = find_item(key, hash, data))) {
            item->value = value;
            return;
        }
        auto index = index_for_hash(hash);
        auto new_key = duplicate_key(key);
        auto new_item = new Item { new_key, value, hash };
        insert_item(m_map, index, new_item);
        m_size++;
    }

    T remove(KeyT key, void *data = nullptr) {
        if (!m_map) return nullptr;
        auto hash = m_hash_fn(key);
        auto index = index_for_hash(hash);
        auto item = m_map[index];
        if (item) {
            // m_map[index] = [item] -> item -> item
            //                ^ this one
            if (hash == item->hash && m_compare_fn(key, item->key, data)) {
                auto value = item->value;
                delete_item(index, item);
                return value;
            }
            auto chained_item = item->next;
            while (chained_item) {
                // m_map[index] = item -> [item] -> item
                //                        ^ this one
                if (hash == chained_item->hash && m_compare_fn(key, chained_item->key, data)) {
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

    void clear() {
        if (!m_map) return;
        for (size_t i = 0; i < m_capacity; i++) {
            auto item = m_map[i];
            m_map[i] = nullptr;
            while (item) {
                auto next_item = item->next;
                free_key(item->key);
                delete item;
                item = next_item;
            }
        }
        m_size = 0;
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

        iterator operator++(int) {
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

        std::pair<KeyT, T> operator*() {
            return std::pair<KeyT, T> { m_item->key, m_item->value };
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
        if (m_size == 0) return end();
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
        return iterator { *this, m_capacity, nullptr };
    }

private:
    // Returns an integer from 0-100
    size_t load_factor() { return m_size * 100 / m_capacity; }

    size_t index_for_hash(size_t hash) const {
        // This is an optimization for hash % capacity that is only possible
        // because capacity is always a power of two.
        assert((m_capacity & (m_capacity - 1)) == 0);
        return hash & (m_capacity - 1);
    }

    size_t calculate_map_size(size_t num_items) {
        size_t target_size = std::max<size_t>(4, num_items) * 100 / HASHMAP_TARGET_LOAD_FACTOR + 1;

        // Round up to the next power of two (if the value is not already a power of two)
        // TODO: This can be replaced by std::bit_ceil in C++20
        target_size--;
        for (size_t i = 1; i < sizeof(size_t) * 8; i *= 2) {
            target_size |= target_size >> i;
        }
        return ++target_size;
    }

    void rehash() {
        auto old_capacity = m_capacity;
        m_capacity = calculate_map_size(m_size);
        auto new_map = new Item *[m_capacity] {};
        for (size_t i = 0; i < old_capacity; i++) {
            auto item = m_map[i];
            while (item) {
                auto next_item = item->next;
                auto new_index = index_for_hash(item->hash);
                insert_item(new_map, new_index, item);
                item = next_item;
            }
        }
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
        free_key(item->key);
        delete item;
        m_size--;
        if (load_factor() < HASHMAP_MIN_LOAD_FACTOR)
            rehash();
    }

    void delete_item(Item *item_before, Item *item) {
        item_before->next = item->next;
        free_key(item->key);
        delete item;
        m_size--;
        if (load_factor() < HASHMAP_MIN_LOAD_FACTOR)
            rehash();
    }

    void copy_items_from(const Hashmap &other) {
        if (!other.m_map) return;
        for (size_t i = 0; i < m_capacity; i++) {
            auto item = other.m_map[i];
            if (item) {
                auto my_item = new Item { *item };
                my_item->key = duplicate_key(item->key);
                m_map[i] = my_item;
                m_size++;
                while (item->next) {
                    item = item->next;
                    my_item->next = new Item { *item };
                    my_item->next->key = duplicate_key(item->key);
                    my_item = my_item->next;
                    m_size++;
                }
            }
        }
    }

    KeyT duplicate_key(KeyT &key) {
        if constexpr (std::is_same_v<char *&, KeyT>) {
            return strdup(key);
        } else {
            return key;
        }
    }

    void free_key(KeyT &key) {
        if constexpr (std::is_same_v<char *&, KeyT>) {
            free(key);
        } else {
            (void)key; // don't warn/error about unused parameter
        }
    }

    size_t m_size { 0 };
    size_t m_capacity { 0 };
    Item **m_map { nullptr };

    HashFn *m_hash_fn { nullptr };
    CompareFn *m_compare_fn { nullptr };
};
}
