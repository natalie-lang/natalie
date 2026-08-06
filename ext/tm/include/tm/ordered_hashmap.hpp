#pragma once

#include "tm/hashmap.hpp"

namespace TM {

// A key-value store like Hashmap that iterates in insertion order, independent
// of how the keys hash. Use it anywhere iteration order is observable.
//
// Entries live in one contiguous array in insertion order, so iteration walks
// linearly through the entries. Lookup goes through a separate array of 32-bit
// indices into the entries ("bins"). Tables small enough to scan directly skip
// the bins entirely. Removal of keys puts a "tombstone" in place of the entry,
// which are dropped when the map is rebuilt.
//
// The only other oddity is that when the key type is a pointer type, the hash
// can be computed directly from the pointer value, so we do not store the hash
// in the entry in order to save space. Note that this means that a null pointer
// is not a valid key since nullptr is used as the tombstone.
template <typename KeyT, typename T = void *>
class OrderedHashmap {
public:
    static constexpr size_t ORDERED_HASHMAP_MAX_SCAN_SIZE = 16;
    static constexpr bool STORES_HASH = !std::is_pointer_v<KeyT>;

    struct HashedEntry {
        size_t hash { 0 };
        KeyT key {};
        T value {};
    };

    struct PointerEntry {
        KeyT key {};
        T value {};
    };

    using Entry = std::conditional_t<STORES_HASH, HashedEntry, PointerEntry>;

    /**
     * Constructs an OrderedHashmap templated with key type and value type.
     * Optionally pass the initial capacity.
     *
     * ```
     * auto map = OrderedHashmap<char *, Thing>();
     * auto key = strdup("foo");
     * map.put(key, Thing(1));
     * assert_eq(1, map.size());
     * free(key);
     * ```
     */
    OrderedHashmap(size_t initial_capacity = 10)
        : m_capacity { static_cast<uint32_t>(initial_capacity) } {
        assert(initial_capacity < BIN_DELETED);
    }

    /**
     * Copies the given OrderedHashmap, preserving insertion order.
     *
     * ```
     * auto map1 = OrderedHashmap<String, Thing>();
     * map1.put("foo", Thing(1));
     * map1.put("bar", Thing(2));
     * auto map2 = OrderedHashmap<String, Thing>(map1);
     * assert_eq(Thing(1), map2.get("foo"));
     * auto it = map2.begin();
     * assert_str_eq("foo", (*it).first);
     * ```
     */
    OrderedHashmap(const OrderedHashmap &other)
        : m_capacity { other.m_capacity } {
        copy_live_entries_from(other);
    }

    /**
     * Creates a new OrderedHashmap from another, and clears the input.
     *
     * ```
     * auto map1 = OrderedHashmap<String, Thing>();
     * map1.put("foo", Thing(1));
     * auto map2 = OrderedHashmap<String, Thing>(std::move(map1));
     * assert_eq(Thing(1), map2.get("foo"));
     * assert(map1.is_empty());
     * ```
     */
    OrderedHashmap(OrderedHashmap &&other)
        : m_entries { other.m_entries }
        , m_bins { other.m_bins }
        , m_capacity { other.m_capacity }
        , m_entries_size { other.m_entries_size }
        , m_deleted { other.m_deleted }
        , m_bins_mask { other.m_bins_mask } {
        other.m_entries = nullptr;
        other.m_entries_size = 0;
        other.m_deleted = 0;
        other.m_bins = nullptr;
        other.m_bins_mask = 0;
    }

    /**
     * Overwrites this OrderedHashmap with another.
     *
     * ```
     * auto map1 = OrderedHashmap<String, Thing>();
     * map1.put("foo", Thing(1));
     * auto map2 = OrderedHashmap<String, Thing>();
     * map2.put("foo", Thing(2));
     * map1 = map2;
     * assert_eq(Thing(2), map1.get("foo"));
     * ```
     */
    OrderedHashmap &operator=(const OrderedHashmap &other) {
        if (this == &other) return *this;
        clear();
        m_capacity = other.m_capacity;
        copy_live_entries_from(other);
        return *this;
    }

    /**
     * Moves data from another OrderedHashmap onto this one.
     *
     * ```
     * auto map1 = OrderedHashmap<String, Thing>();
     * map1.put("foo", Thing(1));
     * auto map2 = OrderedHashmap<String, Thing>();
     * map2.put("foo", Thing(2));
     * map1 = std::move(map2);
     * assert_eq(Thing(2), map1.get("foo"));
     * assert_eq(0, map2.size());
     * ```
     */
    OrderedHashmap &operator=(OrderedHashmap &&other) {
        if (this == &other) return *this;
        clear();
        m_capacity = other.m_capacity;
        m_entries = other.m_entries;
        m_entries_size = other.m_entries_size;
        m_deleted = other.m_deleted;
        m_bins = other.m_bins;
        m_bins_mask = other.m_bins_mask;
        other.m_entries = nullptr;
        other.m_entries_size = 0;
        other.m_deleted = 0;
        other.m_bins = nullptr;
        other.m_bins_mask = 0;
        return *this;
    }

    ~OrderedHashmap() {
        delete[] m_entries;
        delete[] m_bins;
    }

    /**
     * Gets a value from the map stored under the given key.
     * Optionally pass an additional 'data' pointer if your
     * custom compare function requires the extra data
     * parameter.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * map.put("foo", Thing(1));
     * assert_eq(Thing(1), map.get("foo"));
     * ```
     *
     * If there is no value associated with the given key,
     * then a default-constructed object is returned.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * assert_eq(Thing(0), map.get("foo"));
     * ```
     *
     * If there is no value associated with the given key,
     * and your value type is a pointer type, then nullptr
     * is returned.
     *
     * ```
     * auto map = OrderedHashmap<String, const char *>();
     * assert_eq(nullptr, map.get("foo"));
     * ```
     */
    template <typename KeyTArg>
    T get(KeyTArg &&key, void *data = nullptr) const {
        if constexpr (!STORES_HASH)
            assert(key != nullptr);
        auto index = find_entry(key, data);
        if (index != NOT_FOUND)
            return m_entries[index].value;
        if constexpr (std::is_pointer_v<T>)
            return nullptr;
        else
            return {};
    }

    /**
     * Puts the given value at the given key. A new key is appended
     * to the iteration order; overwriting an existing key keeps its
     * original position.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * map.put("foo", Thing(1));
     * map.put("bar", Thing(2));
     * map.put("foo", Thing(3));
     * auto it = map.begin();
     * assert_str_eq("foo", (*it).first);
     * assert_eq(Thing(3), (*it).second);
     * ++it;
     * assert_str_eq("bar", (*it).first);
     * ```
     *
     * If your custom compare function requires the additional 'data'
     * pointer, then pass it as the third parameter.
     */
    template <typename KeyTArg>
    void put(KeyTArg &&key, T value, void *data = nullptr) {
        if constexpr (!STORES_HASH)
            assert(key != nullptr);
        auto index = find_entry(key, data);
        if (index != NOT_FOUND) {
            m_entries[index].value = value;
            return;
        }
        if (!m_entries || m_entries_size == m_capacity) {
            auto live = size();
            if (!m_entries)
                rebuild(std::max<size_t>(4, m_capacity));
            else if (m_deleted > live)
                rebuild(m_capacity);
            else
                rebuild(static_cast<size_t>(m_capacity) * 2);
        }
        index = m_entries_size++;
        if constexpr (STORES_HASH)
            m_entries[index] = Entry { hash_for(key), std::forward<KeyTArg>(key), value };
        else
            m_entries[index] = Entry { std::forward<KeyTArg>(key), value };
        if (m_bins)
            insert_into_bins(index, entry_hash(m_entries[index]));
    }

    /**
     * Removes and returns the value at the given key.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * map.put("foo", Thing(1));
     * assert_eq(Thing(1), map.remove("foo"));
     * assert_eq(Thing(), map.remove("foo"));
     * assert_eq(0, map.size());
     * ```
     *
     * If there is no value associated with the given key,
     * then a default-constructed object is returned.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * assert_eq(Thing(0), map.remove("foo"));
     * ```
     *
     * If there is no value associated with the given key,
     * and your value type is a pointer type, then nullptr
     * is returned.
     *
     * ```
     * auto map = OrderedHashmap<String, const char *>();
     * assert_eq(nullptr, map.remove("foo"));
     * ```
     */
    template <typename KeyTArg>
    T remove(KeyTArg &&key, void *data = nullptr) {
        if constexpr (!STORES_HASH)
            assert(key != nullptr);
        size_t bin = NOT_FOUND;
        auto index = find_entry(key, data, &bin);
        if (index == NOT_FOUND) {
            if constexpr (std::is_pointer_v<T>)
                return nullptr;
            else
                return {};
        }
        auto value = m_entries[index].value;
        m_entries[index] = Entry {};
        m_deleted++;
        if (m_bins) {
            m_bins[bin] = BIN_DELETED;
            if (size() * 4 < m_capacity)
                rebuild(std::max<size_t>(10, size() * 2));
        }
        return value;
    }

    /**
     * Removes all keys/values from the map.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * map.put("foo", Thing(1));
     * assert_eq(1, map.size());
     * map.clear();
     * assert_eq(0, map.size());
     * ```
     */
    void clear() {
        delete[] m_entries;
        delete[] m_bins;
        m_entries = nullptr;
        m_entries_size = 0;
        m_deleted = 0;
        m_bins = nullptr;
        m_bins_mask = 0;
    }

    /**
     * Returns the number of values stored in the map.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * map.put("foo", Thing(1));
     * assert_eq(1, map.size());
     * ```
     */
    size_t size() const { return m_entries_size - m_deleted; }

    /**
     * Returns true if there are zero values stored in the map.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * assert(map.is_empty());
     * map.put("foo", Thing(1));
     * assert_not(map.is_empty());
     * ```
     */
    bool is_empty() const { return size() == 0; }

    template <typename H>
    class iterator {
    public:
        iterator(H &hashmap, size_t index)
            : m_hashmap { hashmap }
            , m_index { index }
            , m_snapshot { hashmap.m_entries } {
            skip_deleted();
        }

        iterator &operator++() {
            check_not_stale();
            ++m_index;
            skip_deleted();
            return *this;
        }

        iterator operator++(int) {
            iterator i = *this;
            ++(*this);
            return i;
        }

        KeyT key() {
            check_not_stale();
            if (m_index < m_hashmap.m_entries_size)
                return m_hashmap.m_entries[m_index].key;
            return nullptr;
        }

        T value() {
            check_not_stale();
            if (m_index < m_hashmap.m_entries_size)
                return m_hashmap.m_entries[m_index].value;
            return nullptr;
        }

        std::pair<KeyT, T> operator*() {
            check_not_stale();
            if (m_index >= m_hashmap.m_entries_size)
                return {};
            auto &entry = m_hashmap.m_entries[m_index];
            return std::pair<KeyT, T> { entry.key, entry.value };
        }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            i1.check_not_stale();
            i2.check_not_stale();
            return &i1.m_hashmap == &i2.m_hashmap && i1.m_index == i2.m_index;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return !(i1 == i2);
        }

    private:
        // A rebuild allocates the new entries array before freeing the old
        // one, so the pointer changes on every rebuild and a stale snapshot
        // means this iterator was used across a mutation that rebuilt.
        void check_not_stale() const {
            assert(m_snapshot == m_hashmap.m_entries);
        }

        // Tombstoned entries stay in the array until the next rebuild; they are
        // never yielded.
        void skip_deleted() {
            while (m_index < m_hashmap.m_entries_size && entry_deleted(m_hashmap.m_entries[m_index]))
                ++m_index;
        }

        H &m_hashmap;
        size_t m_index { 0 };
        const Entry *m_snapshot { nullptr };
    };

    /**
     * Returns an iterator over the map that yields entries in
     * insertion order. The iterator is dereferenced to a std::pair,
     * so you call .first and .second to get the key and value,
     * respectively.
     *
     * ```
     * auto map = OrderedHashmap<String, Thing>();
     * map.put("foo", Thing(1));
     * map.put("bar", Thing(2));
     * map.put("baz", Thing(3));
     * map.remove("bar");
     * auto it = map.begin();
     * assert_str_eq("foo", (*it).first);
     * ++it;
     * assert_str_eq("baz", (*it).first);
     * ++it;
     * assert(it == map.end());
     * ```
     */
    iterator<OrderedHashmap> begin() {
        return iterator<OrderedHashmap> { *this, 0 };
    }

    iterator<const OrderedHashmap> begin() const {
        return iterator<const OrderedHashmap> { *this, 0 };
    }

    iterator<OrderedHashmap> end() {
        return iterator<OrderedHashmap> { *this, m_entries_size };
    }

    iterator<const OrderedHashmap> end() const {
        return iterator<const OrderedHashmap> { *this, m_entries_size };
    }

private:
    static constexpr size_t NOT_FOUND = SIZE_MAX;
    static constexpr uint32_t BIN_EMPTY = UINT32_MAX;
    static constexpr uint32_t BIN_DELETED = UINT32_MAX - 1;
    static constexpr size_t DELETED_HASH = 0;

    // The tombstone marker must never collide with a live entry's stored hash,
    // so the one real hash value equal to it is substituted consistently
    // everywhere.
    template <typename KeyTArg>
    static size_t hash_for(const KeyTArg &key) {
        auto hash = HashKeyHandler<KeyT>::hash(key);
        if (hash == DELETED_HASH)
            return DELETED_HASH + 1;
        return hash;
    }

    static bool entry_deleted(const Entry &entry) {
        if constexpr (STORES_HASH)
            return entry.hash == DELETED_HASH;
        else
            return entry.key == nullptr;
    }

    static size_t entry_hash(const Entry &entry) {
        if constexpr (STORES_HASH)
            return entry.hash;
        else
            return hash_for(entry.key);
    }

    template <typename KeyTArg>
    static bool entry_matches(const Entry &entry, const KeyTArg &key, size_t hash, void *data) {
        if constexpr (STORES_HASH) {
            if (entry.hash != hash)
                return false;
        }
        return HashKeyHandler<KeyT>::compare(key, entry.key, data);
    }

    // Returns the entry index for the given key, or NOT_FOUND. When the table
    // has bins and `bin_out` is given, also reports which bin points at the
    // entry so removal can tombstone both together. The scan path for pointer
    // keys compares keys directly and never hashes at all; a tombstone's null
    // key never matches because null is not a valid key.
    template <typename KeyTArg>
    size_t find_entry(const KeyTArg &key, void *data, size_t *bin_out = nullptr) const {
        if (size() == 0) return NOT_FOUND;
        if (!m_bins) {
            size_t hash = STORES_HASH ? hash_for(key) : 0;
            for (size_t i = 0; i < m_entries_size; i++) {
                if (entry_matches(m_entries[i], key, hash, data))
                    return i;
            }
            return NOT_FOUND;
        }
        auto hash = hash_for(key);
        size_t mask = m_bins_mask;
        for (auto bin = hash & mask;; bin = (bin + 1) & mask) {
            auto index = m_bins[bin];
            if (index == BIN_EMPTY)
                return NOT_FOUND;
            if (index == BIN_DELETED)
                continue;
            if (entry_matches(m_entries[index], key, hash, data)) {
                if (bin_out) *bin_out = bin;
                return index;
            }
        }
    }

    void insert_into_bins(size_t index, size_t hash) {
        size_t mask = m_bins_mask;
        auto bin = hash & mask;
        while (m_bins[bin] != BIN_EMPTY && m_bins[bin] != BIN_DELETED)
            bin = (bin + 1) & mask;
        m_bins[bin] = static_cast<uint32_t>(index);
    }

    // Rebuild the entries array at the given capacity and rebuild the bins when
    // the table is too big to scan linearly.
    void rebuild(size_t new_capacity) {
        assert(new_capacity < BIN_DELETED);
        auto new_entries = new Entry[new_capacity];
        size_t count = 0;
        for (size_t i = 0; i < m_entries_size; i++) {
            if (entry_deleted(m_entries[i])) continue;
            new_entries[count++] = std::move(m_entries[i]);
        }
        delete[] m_entries;
        delete[] m_bins;
        m_entries = new_entries;
        m_entries_size = static_cast<uint32_t>(count);
        m_deleted = 0;
        m_capacity = static_cast<uint32_t>(new_capacity);
        m_bins = nullptr;
        m_bins_mask = 0;
        if (new_capacity > ORDERED_HASHMAP_MAX_SCAN_SIZE)
            build_bins(count);
    }

    // Bins hold twice the entry capacity rounded up to a power of two, so
    // they stay at most half full and an empty slot always ends a probe.
    void build_bins(size_t count) {
        auto capacity = next_power_of_two(static_cast<size_t>(m_capacity) * 2);
        m_bins = new uint32_t[capacity];
        for (size_t i = 0; i < capacity; i++)
            m_bins[i] = BIN_EMPTY;
        m_bins_mask = static_cast<uint32_t>(capacity - 1);
        for (size_t i = 0; i < count; i++)
            insert_into_bins(i, entry_hash(m_entries[i]));
    }

    // Copies the other map's live entries densely. Stored hashes are reused
    // and pointer-key hashes are derived from the keys, so keys are never
    // compared and key types whose compare needs the data pointer copy safely.
    void copy_live_entries_from(const OrderedHashmap &other) {
        if (!other.m_entries || other.size() == 0) return;
        m_entries = new Entry[m_capacity];
        size_t count = 0;
        for (size_t i = 0; i < other.m_entries_size; i++) {
            auto &entry = other.m_entries[i];
            if (entry_deleted(entry)) continue;
            m_entries[count++] = entry;
        }
        m_entries_size = static_cast<uint32_t>(count);
        if (m_capacity > ORDERED_HASHMAP_MAX_SCAN_SIZE)
            build_bins(count);
    }

    // TODO: This can be replaced by std::bit_ceil in C++20.
    static size_t next_power_of_two(size_t target_size) {
        if (target_size <= 1)
            return 1;
        return size_t { 1 } << (sizeof(unsigned long long) * 8 - __builtin_clzll(target_size - 1));
    }

    Entry *m_entries { nullptr };
    uint32_t *m_bins { nullptr };
    uint32_t m_capacity { 0 };
    uint32_t m_entries_size { 0 };
    uint32_t m_deleted { 0 };
    uint32_t m_bins_mask { 0 };
};

}
