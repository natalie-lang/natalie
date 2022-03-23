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

    /**
     * Constructs a Hashmap templated with key type and value type.
     * Pass the hash function, compare function, and optionally,
     * the initial capacity.
     *
     * ```
     * auto hash_fn = &Hashmap<char*>::hash_str;
     * auto compare_fn = &Hashmap<char*>::compare_str;
     * auto map = Hashmap<char*, Thing>(hash_fn, compare_fn);
     * auto key = strdup("foo");
     * map.put(key, Thing(1));
     * assert_eq(1, map.size());
     * free(key);
     * ```
     */
    Hashmap(HashFn hash_fn, CompareFn compare_fn, size_t initial_capacity = 10)
        : m_capacity { calculate_map_size(initial_capacity) }
        , m_hash_fn { hash_fn }
        , m_compare_fn { compare_fn } { }

    /**
     * Constructs a Hashmap templated with key type and value type.
     * By default, the Hashmap is configured to hash pointer keys
     * using a simple identity function.
     *
     * ```
     * auto map = Hashmap<char*, Thing>();
     * auto key1 = strdup("foo");
     * auto key2 = strdup(key1); // different pointer
     * map.put(key1, Thing(1));
     * map.put(key2, Thing(2));
     * assert_eq(2, map.size()); // two different keys
     * free(key1);
     * free(key2);
     * ```
     *
     * To hash based on the C string's contents, then specify
     * HashType::String.
     *
     * ```
     * auto map = Hashmap<char*, Thing>(HashType::String);
     * auto key1 = strdup("foo");
     * auto key2 = strdup(key1); // different pointer
     * map.put(key1, Thing(1));
     * map.put(key2, Thing(2));
     * assert_eq(1, map.size()); // same key
     * free(key1);
     * free(key2);
     * ```
     *
     * When using a C string (char*) as the key, the char* is
     * duplicated inside the Hashmap and managed for the lifetime
     * of that key and/or hashmap. You do not need to keep
     * the char* available or worry about freeing it.
     *
     * ```
     * auto map = Hashmap<char*, Thing>(HashType::String);
     * auto key = strdup("foo");
     * map.put(key, Thing(1));
     * free(key);
     * auto key2 = strdup("foo");
     * assert_eq(Thing(1), map.get(key2));
     * free(key2);
     * ```
     *
     * To use a TM::String as the key, specify HashType::TMString.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * auto key1 = String("foo");
     * auto key2 = String("foo");
     * map.put(key1, Thing(1));
     * map.put(key2, Thing(2));
     * assert_eq(1, map.size()); // same key
     * ```
     *
     * To use the Hashmap as a hash set (keys only), then you
     * can specify only the key type and use set()/get().
     *
     * ```
     * auto map = Hashmap<String>(HashType::TMString);
     * map.set("foo");
     * assert(map.get("foo"));
     * ```
     */
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

    /**
     * Returns a hash value for the given pointer as if it were
     * just a 64-bit number. The contents of the pointer are
     * not examined.
     *
     * ```
     * auto key1 = strdup("foo");
     * auto key2 = strdup("foo");
     * assert_neq(
     *   Hashmap<char*>::hash_ptr(key1),
     *   Hashmap<char*>::hash_ptr(key2)
     * );
     * assert_eq(
     *   Hashmap<char*>::hash_ptr(key1),
     *   Hashmap<char*>::hash_ptr(key1)
     * );
     * free(key1);
     * free(key2);
     * ```
     */
    static size_t hash_ptr(KeyT &ptr) {
        if constexpr (std::is_pointer_v<KeyT>)
            return HashmapUtils::hashmap_hash_ptr((uintptr_t)ptr);
        else
            return 0;
    }

    /**
     * Returns true if the two given pointers are the same.
     * The contents of the pointed-to object are not examined.
     * Null must be passed as the third argument.
     *
     * ```
     * auto key1 = strdup("foo");
     * auto key2 = strdup("foo");
     * assert_not(Hashmap<char*>::compare_ptr(key1, key2, nullptr));
     * assert(Hashmap<char*>::compare_ptr(key1, key1, nullptr));
     * free(key1);
     * free(key2);
     * ```
     */
    static bool compare_ptr(KeyT &a, KeyT &b, void *) {
        return a == b;
    }

    /**
     * Returns a hash value for the given C string based on its contents.
     *
     * ```
     * auto key1 = strdup("foo");
     * auto key2 = strdup("foo");
     * auto key3 = strdup("bar");
     * assert_eq(
     *   Hashmap<char*>::hash_str(key1),
     *   Hashmap<char*>::hash_str(key2)
     * );
     * assert_neq(
     *   Hashmap<char*>::hash_ptr(key1),
     *   Hashmap<char*>::hash_ptr(key3)
     * );
     * free(key1);
     * free(key2);
     * free(key3);
     * ```
     */
    static size_t hash_str(KeyT &ptr) {
        if constexpr (std::is_pointer_v<KeyT>) {
            auto str = String((const char *)(ptr));
            return str.djb2_hash();
        } else {
            return 0;
        }
    }

    /**
     * Returns true if the two given C strings have the same contents.
     * Null must be passed as the third argument.
     *
     * ```
     * auto key1 = strdup("foo");
     * auto key2 = strdup("foo");
     * auto key3 = strdup("bar");
     * assert(Hashmap<char*>::compare_str(key1, key2, nullptr));
     * assert_not(Hashmap<char*>::compare_str(key1, key3, nullptr));
     * free(key1);
     * free(key2);
     * free(key3);
     * ```
     */
    static bool compare_str(KeyT &a, KeyT &b, void *) {
        if constexpr (std::is_pointer_v<KeyT>)
            return strcmp((const char *)(a), (const char *)(b)) == 0;
        else
            return false;
    }

    /**
     * Returns a hash value for the given TM::String based on its contents.
     *
     * ```
     * auto key1 = String("foo");
     * auto key2 = String("foo");
     * auto key3 = String("bar");
     * assert_eq(
     *   Hashmap<String>::hash_tm_str(key1),
     *   Hashmap<String>::hash_tm_str(key2)
     * );
     * assert_neq(
     *   Hashmap<String>::hash_tm_str(key1),
     *   Hashmap<String>::hash_tm_str(key3)
     * );
     * ```
     */
    static size_t hash_tm_str(KeyT &ptr) {
        if constexpr (std::is_pointer_v<KeyT>) {
            return 0;
        } else {
            auto str = (const String &)(ptr);
            return str.djb2_hash();
        }
    }

    /**
     * Returns true if the two given TM:Strings have the same contents.
     * Null must be passed as the third argument.
     *
     * ```
     * auto key1 = String("foo");
     * auto key2 = String("foo");
     * auto key3 = String("bar");
     * assert(Hashmap<String>::compare_tm_str(key1, key2, nullptr));
     * assert_not(Hashmap<String>::compare_tm_str(key1, key3, nullptr));
     * ```
     */
    static bool compare_tm_str(KeyT &a, KeyT &b, void *) {
        if constexpr (std::is_pointer_v<KeyT>) {
            return false;
        } else {
            return ((const String &)a) == ((const String &)b);
        }
    }

    /**
     * Copies the given Hashmap.
     *
     * ```
     * auto map1 = Hashmap<String, Thing>(HashType::TMString);
     * map1.put("foo", Thing(1));
     * auto map2 = Hashmap<String, Thing>(map1);
     * assert_eq(Thing(1), map2.get("foo"));
     * ```
     */
    Hashmap(const Hashmap &other)
        : m_capacity { other.m_capacity }
        , m_hash_fn { other.m_hash_fn }
        , m_compare_fn { other.m_compare_fn } {
        m_map = new Item *[m_capacity] {};
        copy_items_from(other);
    }

    /**
     * Overwrites this Hashmap with another.
     *
     * ```
     * auto map1 = Hashmap<String, Thing>(HashType::TMString);
     * map1.put("foo", Thing(1));
     * auto map2 = Hashmap<String, Thing>(HashType::TMString);
     * map2.put("foo", Thing(2));
     * map1 = map2;
     * assert_eq(Thing(2), map1.get("foo"));
     * ```
     */
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

    /**
     * Moves data from another Hashmap onto this one.
     *
     * ```
     * auto map1 = Hashmap<String, Thing>(HashType::TMString);
     * map1.put("foo", Thing(1));
     * auto map2 = Hashmap<String, Thing>(HashType::TMString);
     * map2.put("foo", Thing(2));
     * map1 = std::move(map2);
     * assert_eq(Thing(2), map1.get("foo"));
     * assert_eq(0, map2.size());
     * ```
     */
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

    /**
     * Gets a value from the map stored under the given key.
     * Optionally pass an additional 'data' pointer if your
     * custom compare function requires the extra data
     * parameter. (The built-in compare function does not
     * use the data pointer.)
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * map.put("foo", Thing(1));
     * assert_eq(Thing(1), map.get("foo"));
     * ```
     *
     * If there is no value associated with the given key,
     * then a default-constructed object is returned.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * assert_eq(Thing(0), map.get("foo"));
     * ```
     *
     * If there is no value associated with the given key,
     * and your value type is a pointer type, then nullptr
     * is returned.
     *
     * ```
     * auto map = Hashmap<String, const char*>(HashType::TMString);
     * assert_eq(nullptr, map.get("foo"));
     * ```
     */
    T get(KeyT key, void *data = nullptr) const {
        auto hash = m_hash_fn(key);
        auto item = find_item(key, hash, data);
        if (item)
            return item->value;
        if constexpr (std::is_pointer_v<T>)
            return nullptr;
        else
            return {};
    }

    /**
     * Finds and returns an Item* (the internal container type
     * used by Hashmap) based on the given key and hash.
     * Optionally pass a third data pointer if your custom
     * compare function requires it.
     *
     * Use this method if you already have computed the hash value
     * and/or need to access the internal Item structure directly.
     *
     * ```
     * auto key = String("foo");
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * map.put(key, Thing(1));
     * auto hash = Hashmap<String>::hash_tm_str(key);
     * auto item = map.find_item(key, hash);
     * assert_eq(Thing(1), item->value);
     * ```
     *
     * If the item is not found, then null is returned.
     *
     * ```
     * auto key = String("foo");
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * auto hash = Hashmap<String>::hash_tm_str(key);
     * auto item = map.find_item(key, hash);
     * assert_eq(nullptr, item);
     * ```
     */
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

    /**
     * Sets a key in the Hashmap as if it were a hash set.
     * Use this if you don't care about storing/retrieving values.
     *
     * ```
     * auto map = Hashmap<String>(HashType::TMString);
     * map.set("foo");
     * assert(map.get("foo"));
     * assert_not(map.get("bar"));
     * ```
     */
    void set(KeyT key) {
        put(key, this); // we just put a placeholder value, a pointer to this Hashmap
    }

    /**
     * Puts the given value at the given key in the Hashmap.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * map.put("foo", Thing(1));
     * assert_eq(Thing(1), map.get("foo"));
     * map.put("foo", Thing(2));
     * assert_eq(Thing(2), map.get("foo"));
     * ```
     *
     * If your custom compare function requires the additional 'data'
     * pointer, then pass it as the third parameter.
     */
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

    /**
     * Removes and returns the value at the given key.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * map.put("foo", Thing(1));
     * assert_eq(Thing(1), map.remove("foo"));
     * assert_eq(Thing(), map.remove("foo"));
     * ```
     *
     * If there is no value associated with the given key,
     * then a default-constructed object is returned.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * assert_eq(Thing(0), map.remove("foo"));
     * ```
     *
     * If there is no value associated with the given key,
     * and your value type is a pointer type, then nullptr
     * is returned.
     *
     * ```
     * auto map = Hashmap<String, const char*>(HashType::TMString);
     * assert_eq(nullptr, map.remove("foo"));
     * ```
     */
    T remove(KeyT key, void *data = nullptr) {
        if (!m_map) {
            if constexpr (std::is_pointer_v<T>)
                return nullptr;
            else
                return {};
        }
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
        if constexpr (std::is_pointer_v<T>)
            return nullptr;
        else
            return {};
    }

    /**
     * Removes all keys/values from the Hashmap.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * map.put("foo", Thing(1));
     * assert_eq(1, map.size());
     * map.clear();
     * assert_eq(0, map.size());
     * ```
     */
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

    /**
     * Returns the number of values stored in the Hashmap.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * map.put("foo", Thing(1));
     * assert_eq(1, map.size());
     * ```
     */
    size_t size() const { return m_size; }

    /**
     * Returns true if there are zero values stored in the Hashmap.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * assert(map.is_empty());
     * map.put("foo", Thing(1));
     * assert_not(map.is_empty());
     * ```
     */
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

    /**
     * Returns an iterator over the Hashmap.
     * The iterator is dereferenced to a std::pair,
     * so you call .first and .second to get the key and
     * value, respectively.
     *
     * ```
     * auto map = Hashmap<String, Thing>(HashType::TMString);
     * map.put("foo", Thing(1));
     * for (std::pair item : map) {
     *     assert_str_eq("foo", item.first);
     *     assert_eq(Thing(1), item.second);
     * }
     * ```
     */
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
        if constexpr (std::is_same_v<char *, KeyT> || std::is_same_v<const char *, KeyT>) {
            return strdup(key);
        } else {
            return key;
        }
    }

    void free_key(KeyT &key) {
        if constexpr (std::is_same_v<char *, KeyT> || std::is_same_v<const char *, KeyT>) {
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
