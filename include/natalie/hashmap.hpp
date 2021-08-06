/*
 * Copyright (c) 2016-2020 David Leeds
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Copyright (c) 2016-2018 David Leeds <davidesleeds@gmail.com>
 *
 * ----
 *
 * Large portions of this file were copied from David Leeds' hashmap library
 * made available at: https://github.com/DavidLeeds/hashmap
 *
 * Modifications to work better with Natalie made by Tim Morgan, 2021.
 */

#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "natalie/gc.hpp"
#include "natalie/types.hpp"

namespace Natalie {

class Env;

/*
 * Define HASHMAP_METRICS to compile in performance analysis
 * functions for use in assessing hash function performance.
 */
/* #define HASHMAP_METRICS */

/*
 * Define HASHMAP_NOASSERT to compile out all assertions used internally.
 */
/* #define HASHMAP_NOASSERT */

struct hashmap_iter;

struct hashmap_entry {
    void *key;
    void *data;
#ifdef HASHMAP_METRICS
    size_t num_collisions;
#endif
};

/*
 * The hashmap state structure.
 */
struct hashmap {
    size_t table_size_init { 0 };
    size_t table_size { 0 };
    size_t num_entries { 0 };
    struct hashmap_entry *table { nullptr };
    nat_int_t (*hash)(const void *) { nullptr };
    int (*key_compare)(const void *, const void *, Env *) { nullptr };
    void *(*key_alloc)(const void *) { nullptr };
    void (*key_free)(void *) { nullptr };
};

typedef struct hashmap hashmap;

/*
 * Initialize an empty hashmap.
 *
 * hash_func should return an even distribution of numbers between 0
 * and SIZE_MAX varying on the key provided.  If set to NULL, the default
 * case-sensitive string hash function is used: hashmap_hash_string
 *
 * key_compare_func should return 0 if the keys match, and non-zero otherwise.
 * If set to NULL, the default case-sensitive string comparator function is
 * used: hashmap_compare_string
 *
 * initial_size is optional, and may be set to the max number of entries
 * expected to be put in the hash table.  This is used as a hint to
 * pre-allocate the hash table to the minimum size needed to avoid
 * gratuitous rehashes.  If initial_size is 0, a default size will be used.
 *
 * Returns 0 on success and -errno on failure.
 */
int hashmap_init(struct hashmap *map, nat_int_t (*hash_func)(const void *),
    int (*key_compare_func)(const void *, const void *, Env *),
    size_t initial_size);

/*
 * Free the hashmap and all associated memory.
 */
void hashmap_destroy(struct hashmap *map);

/*
 * Enable internal memory allocation and management of hash keys.
 */
void hashmap_set_key_alloc_funcs(struct hashmap *map,
    void *(*key_alloc_func)(const void *),
    void (*key_free_func)(void *));

/*
 * Add an entry to the hashmap.  If an entry with a matching key already
 * exists, the existing data pointer is overwritten with the new data.
 * Returns NULL if memory allocation failed.
 */
void *hashmap_put(struct hashmap *map, const void *key, void *data, Env *env);

/*
 * Return the data pointer, or NULL if no entry exists.
 */
void *hashmap_get(const struct hashmap *map, const void *key, Env *env);

/*
 * Remove an entry with the specified key from the map.
 * Returns the data pointer, or NULL, if no entry was found.
 */
void *hashmap_remove(struct hashmap *map, const void *key, Env *env);

/*
 * Remove all entries.
 */
void hashmap_clear(struct hashmap *map);

/*
 * Remove all entries and reset the hash table to its initial size.
 */
void hashmap_reset(struct hashmap *map);

/*
 * Return the number of entries in the hash map.
 */
size_t hashmap_size(const struct hashmap *map);

/*
 * Return the internal data struct for holding the key and data.
 */
struct hashmap_entry *hashmap_entry_find(const struct hashmap *map,
    const void *key, bool find_empty, Env *env);

/*
 * Get a new hashmap iterator.  The iterator is an opaque
 * pointer that may be used with hashmap_iter_*() functions.
 * Hashmap iterators are INVALID after a put or remove operation is performed.
 * hashmap_iter_remove() allows safe removal during iteration.
 */
struct hashmap_iter *hashmap_iter(const struct hashmap *map);

/*
 * Return an iterator to the next hashmap entry.  Returns NULL if there are
 * no more entries.
 */
struct hashmap_iter *hashmap_iter_next(const struct hashmap *map,
    const struct hashmap_iter *iter);

/*
 * Remove the hashmap entry pointed to by this iterator and returns an
 * iterator to the next entry.  Returns NULL if there are no more entries.
 */
struct hashmap_iter *hashmap_iter_remove(struct hashmap *map,
    const struct hashmap_iter *iter);

/*
 * Return the key of the entry pointed to by the iterator.
 */
const void *hashmap_iter_get_key(const struct hashmap_iter *iter);

/*
 * Return the data of the entry pointed to by the iterator.
 */
void *hashmap_iter_get_data(const struct hashmap_iter *iter);

/*
 * Set the data pointer of the entry pointed to by the iterator.
 */
void hashmap_iter_set_data(const struct hashmap_iter *iter, void *data);

/*
 * Invoke func for each entry in the hashmap.  Unlike the hashmap_iter_*()
 * interface, this function supports calls to hashmap_remove() during iteration.
 * However, it is an error to put or remove an entry other than the current one,
 * and doing so will immediately halt iteration and return an error.
 * Iteration is stopped if func returns non-zero.  Returns func's return
 * value if it is < 0, otherwise, 0.
 */
int hashmap_foreach(const struct hashmap *map,
    int (*func)(const void *, void *, void *), void *arg);

nat_int_t hashmap_hash_ptr(const void *key);

/*
 * Default hash function for string keys.
 * This is an implementation of the well-documented Jenkins one-at-a-time
 * hash function.
 */
nat_int_t hashmap_hash_string(const void *key);

int hashmap_compare_ptr(const void *a, const void *b, Env *env);

/*
 * Default key comparator function for string keys.
 */
int hashmap_compare_string(const void *a, const void *b, Env *env);

/*
 * Default key allocation function for string keys.  Use free() for the
 * key_free_func.
 */
void *hashmap_alloc_key_string(const void *key);

/*
 * Case insensitive hash function for string keys.
 */
size_t hashmap_hash_string_i(const void *key);

/*
 * Case insensitive key comparator function for string keys.
 */
int hashmap_compare_string_i(const void *a, const void *b);

#ifdef HASHMAP_METRICS
/*
 * Return the load factor.
 */
double hashmap_load_factor(const struct hashmap *map);

/*
 * Return the average number of collisions per entry.
 */
double hashmap_collisions_mean(const struct hashmap *map);

/*
 * Return the variance between entry collisions.  The higher the variance,
 * the more likely the hash function is poor and is resulting in clustering.
 */
double hashmap_collisions_variance(const struct hashmap *map);

#endif

enum class HashmapKeyType {
    Pointer,
    String,
};

using HashFn = nat_int_t(const void *);
using CompareFn = int(const void *, const void *, Env *);

template <typename KeyT, typename T = void *>
class Hashmap {
public:
    Hashmap(int initial_capacity = 10, HashmapKeyType key_type = HashmapKeyType::Pointer) {
        if (key_type == HashmapKeyType::String) {
            hashmap_init(&m_map, hashmap_hash_string, hashmap_compare_string, initial_capacity);
            hashmap_set_key_alloc_funcs(&m_map, hashmap_alloc_key_string, free);
        } else {
            hashmap_init(&m_map, hashmap_hash_ptr, hashmap_compare_ptr, initial_capacity);
        }
    }

    Hashmap(HashFn *hash_fn, CompareFn *compare_fn, int initial_capacity = 10) {
        hashmap_init(&m_map, hash_fn, compare_fn, initial_capacity);
    }

    Hashmap(const Hashmap &other)
        : m_map { other.m_map } {
        m_map.table = static_cast<struct hashmap_entry *>(malloc(m_map.table_size * sizeof(struct hashmap_entry)));
        memcpy(m_map.table, other.m_map.table, m_map.table_size * sizeof(struct hashmap_entry));
    }

    Hashmap &operator=(const Hashmap &other) {
        assert(!m_map.key_free); // don't know how to free keys with copy constructor yet (not needed?)
        free(m_map.table);
        m_map = other.m_map;
        m_map.table = static_cast<struct hashmap_entry *>(malloc(m_map.table_size * sizeof(struct hashmap_entry)));
        memcpy(m_map.table, other.m_map.table, m_map.table_size * sizeof(struct hashmap_entry));
        return *this;
    }

    ~Hashmap() {
        hashmap_destroy(&m_map);
    }

    T get(KeyT key, Env *env = nullptr) {
        return static_cast<T>(hashmap_get(&m_map, key, env));
    }

    struct hashmap_entry *find_entry(KeyT key, Env *env = nullptr) {
        return hashmap_entry_find(&m_map, key, false, env);
    }

    void set(KeyT key) {
        hashmap_put(&m_map, key, this, nullptr);
    }

    void put(KeyT key, T val, Env *env = nullptr) {
        assert(hashmap_put(&m_map, key, val, env));
    }

    T remove(KeyT key, Env *env = nullptr) {
        return static_cast<T>(hashmap_remove(&m_map, key, env));
    }

    size_t size() const { return m_map.num_entries; }
    bool is_empty() const { return m_map.num_entries == 0; }

    class iterator {
    public:
        iterator(hashmap *map, struct hashmap_iter *iter)
            : m_map { map }
            , m_iter { iter } { }

        iterator operator++() {
            m_iter = hashmap_iter_next(m_map, m_iter);
            return *this;
        }

        iterator operator++(int _) {
            iterator i = *this;
            m_iter = hashmap_iter_next(m_map, m_iter);
            return i;
        }

        KeyT key() { return static_cast<KeyT>(hashmap_iter_get_key(m_iter)); }
        T val() { return static_cast<T>(hashmap_iter_get_data(m_iter)); }

        std::pair<KeyT, T> operator*() {
            return std::pair(
                static_cast<KeyT>(const_cast<void *>(hashmap_iter_get_key(m_iter))),
                static_cast<T>(const_cast<void *>(hashmap_iter_get_data(m_iter))));
        }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_iter == i2.m_iter;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_iter != i2.m_iter;
        }

    private:
        hashmap *m_map;
        struct hashmap_iter *m_iter;
    };

    iterator begin() {
        return iterator { &m_map, hashmap_iter(&m_map) };
    }

    iterator end() {
        return iterator { &m_map, nullptr };
    }

private:
    hashmap m_map {};
};

}
