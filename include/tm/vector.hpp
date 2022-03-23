#pragma once

#include <algorithm>
#include <assert.h>
#include <initializer_list>
#include <stddef.h>
#include <string.h>
#include <type_traits>

namespace TM {

const int VECTOR_GROW_FACTOR = 2;
const int VECTOR_MIN_CAPACITY = 10;

template <typename T>
class Vector {
public:
    /**
     * Constructs an empty vector with the default capacity.
     *
     * ```
     * auto vec = Vector<char> {};
     * assert_eq(0, vec.size());
     * assert_eq(10, vec.capacity());
     * ```
     */
    Vector()
        : m_capacity { VECTOR_MIN_CAPACITY }
        , m_data { array_of_size(VECTOR_MIN_CAPACITY) } { }

    /**
     * Constructs an empty vector with the given capacity.
     *
     * ```
     * auto vec = Vector<char>(1);
     * assert_eq(0, vec.size());
     * assert_eq(1, vec.capacity());
     * ```
     */
    Vector(size_t initial_capacity)
        : m_capacity { initial_capacity }
        , m_data { array_of_size(initial_capacity) } { }

    /**
     * Constructs a vector with the given size
     * and every slot filled with the given filler object.
     *
     * ```
     * auto vec = Vector<char>(10, 'a');
     * assert_eq(10, vec.size());
     * assert_eq('a', vec[0]);
     * assert_eq('a', vec[9]);
     * ```
     */
    Vector(size_t size, T filler)
        : m_size { size }
        , m_capacity { size }
        , m_data { array_of_size(size) } {
        fill(0, size, filler);
    }

    /**
     * Constructs a vector with the given list of items.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * assert_eq(3, vec.size());
     * ```
     */
    Vector(std::initializer_list<T> list)
        : m_capacity { list.size() }
        , m_data { array_of_size(list.size()) } {
        for (auto v : list) {
            push(v);
        }
    }

    /**
     * Constructs a vector by copying data from another vector.
     *
     * ```
     * auto vec1 = Vector<char> { 'a', 'b', 'c' };
     * auto vec2 = Vector<char>(vec1);
     * assert_eq(3, vec2.size());
     * ```
     */
    Vector(const Vector &other)
        : m_size { other.m_size }
        , m_capacity { other.m_size }
        , m_data { array_of_size(other.m_size) } {
        copy_data(m_data, other.m_data, other.m_size);
    }

    ~Vector() {
        delete_memory();
    }

    /**
     * Deletes all the items in the vector.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * vec.clear();
     * assert_eq(0, vec.size());
     * ```
     */
    void clear() { m_size = 0; }

    /**
     * Returns a new vector from the given offset and count.
     *
     * ```
     * auto vec1 = Vector<char> { 'a', 'b', 'c', 'd', 'e' };
     * auto vec2 = vec1.slice(2, 2);
     * assert_eq(2, vec2.size());
     * assert_eq('c', vec2[0]);
     * assert_eq('d', vec2[1]);
     * ```
     *
     * If `count` is not specified, then the returned vector
     * will include all items from the index to the end.
     *
     * ```
     * auto vec1 = Vector<char> { 'a', 'b', 'c', 'd', 'e' };
     * auto vec2 = vec1.slice(2);
     * assert_eq(3, vec2.size());
     * assert_eq('c', vec2[0]);
     * assert_eq('d', vec2[1]);
     * assert_eq('e', vec2[2]);
     * ```
     */
    Vector slice(size_t offset, size_t count = 0) {
        if (count == 0 || offset + count > m_size) {
            count = m_size - offset;
        }
        if (offset >= m_size || count == 0) {
            return {};
        }
        T *data = array_of_size(count);
        copy_data(data, m_data + offset, count);
        return { count, count, data };
    }

    /**
     * Overwrites the vector data with that of another vector.
     *
     * ```
     * auto vec1 = Vector<char> { 'a', 'b', 'c' };
     * auto vec2 = Vector<char> { 'x', 'y', 'z' };
     * vec1 = vec2;
     * assert_eq('y', vec1[1]);
     * ```
     *
     * This method handles non-trivially-copyable types as well:
     *
     * ```
     * auto t1 = Thing(1);
     * auto t2 = Thing(2);
     * auto t3 = Thing(3);
     * auto vec1 = Vector<Thing> { t1, t2, t3 };
     * auto vec2 = Vector<Thing> { t3, t2, t1 };
     * vec1 = vec2;
     * assert_eq(3, vec1.size());
     * assert_eq(t3, vec1[0]);
     * assert_eq(t2, vec1[1]);
     * assert_eq(t1, vec1[2]);
     * ```
     */
    Vector &operator=(Vector &other) {
        clear();
        grow_at_least(other.m_size);
        copy_data(m_data, other.m_data, other.m_size);
        m_size = other.m_size;
        return *this;
    }

    /**
     * Moves another vector's data into this one.
     * The other vector is left empty.
     *
     * ```
     * auto vec1 = Vector<char> { 'a', 'b', 'c' };
     * auto vec2 = Vector<char> { 'x', 'y', 'z' };
     * vec1 = std::move(vec2);
     * assert_eq('y', vec1[1]);
     * assert_eq(0, vec2.size());
     * ```
     */
    Vector &operator=(Vector &&other) {
        delete_memory();
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_data = other.m_data;
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
        return *this;
    }

    /**
     * Returns a reference to the value at the given index.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * assert_eq('b', vec[1]);
     * ```
     *
     * WARNING: This method does *not* check that the given
     * index is within the bounds of the vector!
     */
    T &operator[](size_t index) const {
        return m_data[index];
    }

    /**
     * Returns a reference to the value at the given index.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * assert_eq('b', vec.at(1));
     * ```
     *
     * If the vector is empty, then this method aborts.
     *
     * ```should_abort
     * auto vec = Vector<char> {};
     * vec.at(0);
     * ```
     */
    T &at(size_t index) const {
        assert(index < m_size);
        return m_data[index];
    }

    /**
     * Returns the value at the front (index 0).
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * assert_eq('a', vec.first());
     * ```
     *
     * If the vector is empty, then this method aborts.
     *
     * ```should_abort
     * auto vec = Vector<char> {};
     * vec.first();
     * ```
     */
    T first() {
        assert(m_size != 0);
        return m_data[0];
    }

    /**
     * Returns the value at the end (at index size() - 1).
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * assert_eq('c', vec.last());
     * ```
     *
     * If the vector is empty, then this method aborts.
     *
     * ```should_abort
     * auto vec = Vector<char> {};
     * vec.last();
     * ```
     */
    T last() {
        assert(m_size != 0);
        return m_data[m_size - 1];
    }

    /**
     * Removes and returns the value at the end (at index size() - 1).
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * assert_eq('c', vec.pop());
     * ```
     *
     * If the vector is empty, then this method aborts.
     *
     * ```should_abort
     * auto vec = Vector<char> {};
     * vec.pop();
     * ```
     */
    T pop() {
        assert(m_size != 0);
        return m_data[--m_size];
    }

    /**
     * Pushes (appends) a value at the end (at index size()).
     *
     * ```
     * auto vec = Vector<char> { 'a' };
     * vec.push('b');
     * assert_eq('a', vec[0]);
     * assert_eq('b', vec[1]);
     * ```
     */
    void push(T val) {
        size_t len = m_size;
        if (m_size >= m_capacity) {
            grow_at_least(m_size + 1);
        }
        m_size++;
        m_data[len] = val;
    }

    /**
     * Pushes (inserts) a value at the front (index 0).
     *
     * ```
     * auto vec = Vector<char> { 'b' };
     * vec.push_front('a');
     * assert_eq('a', vec[0]);
     * ```
     */
    void push_front(T val) {
        insert(0, val);
    }

    /**
     * Inserts a value at the given index.
     * Index must be <= the size of the vector,
     * i.e. you can only increase the vector size by one.
     *
     * ```
     * auto vec = Vector<char>(0);
     * vec.insert(0, 'a');
     * vec.insert(1, 'b');
     * vec.insert(0, 'c');
     * assert_eq(3, vec.size());
     * assert_eq('c', vec[0]);
     * assert_eq('a', vec[1]);
     * assert_eq('b', vec[2]);
     * ```
     *
     * This method aborts if the given index is more than 1
     * past the end.
     *
     * ```should_abort
     * auto vec = Vector<char> { 0 };
     * vec.insert(25, 'z'); // beyond the end
     * ```
     *
     * This method handles non-trivially-copyable types as well:
     *
     * ```
     * auto t1 = Thing(1);
     * auto t2 = Thing(2);
     * auto t3 = Thing(3);
     * auto vec = Vector<Thing> { t1, t2 };
     * vec.insert(0, t3);
     * assert_eq(3, vec.size());
     * assert_eq(t3, vec[0]);
     * assert_eq(t1, vec[1]);
     * assert_eq(t2, vec[2]);
     * ```
     */
    void insert(size_t index, T val) {
        assert(index <= m_size);
        grow_at_least(m_size + 1);

        if (index == m_size) {
            m_size++;
            m_data[index] = val;
            return;
        }

        if constexpr (std::is_trivially_copyable<T>::value) {
            memmove(m_data + index + 1, m_data + index, (m_size - index) * sizeof(T));
        } else {
            for (size_t i = m_size - 1; i > index; --i)
                m_data[i + 1] = m_data[i];
            m_data[index + 1] = m_data[index];
        }

        m_data[index] = val;
        m_size++;
    }

    /**
     * Removes the item from the front (index 0)
     * and shifts all remaining items over.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * assert_eq('a', vec.pop_front());
     * ```
     *
     * This method aborts if the vector is empty.
     *
     * ```should_abort
     * auto vec = Vector<char> {};
     * vec.pop_front();
     * ```
     */
    T pop_front() {
        T val = m_data[0];
        remove(0);
        return val;
    }

    /**
     * Removes an item from the vector at the given index;
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * vec.remove(1);
     * assert_eq('c', vec[1]);
     * ```
     *
     * This method aborts if the index is past the end.
     *
     * ```should_abort
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * vec.remove(3);
     * ```
     *
     * This method handles non-trivially-copyable types as well:
     *
     * ```
     * auto t1 = Thing(1);
     * auto t2 = Thing(2);
     * auto vec = Vector<Thing> { t1, t2 };
     * vec.remove(0);
     * assert_eq(1, vec.size());
     * assert_eq(t2, vec[0]);
     * ```
     */
    void remove(size_t index) {
        assert(m_size > index);

        --m_size;

        if (index == m_size)
            return;

        if constexpr (std::is_trivially_copyable<T>::value) {
            memmove(m_data + index, m_data + index + 1, (m_size - index) * sizeof(T));
        } else {
            for (size_t i = index; i < m_size; ++i)
                m_data[i] = m_data[i + 1];
        }
    }

    /**
     * Returns true if the vector has no items.
     *
     * ```
     * auto vec1 = Vector<char> { 'a', 'b', 'c' };
     * assert_eq(false, vec1.is_empty());
     *
     * auto vec2 = Vector<char> {};
     * assert_eq(true, vec2.is_empty());
     * ```
     */
    bool is_empty() const { return m_size == 0; }

    /**
     * Returns the number of items stored in the vector.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * assert_eq(3, vec.size());
     * ```
     */
    size_t size() const { return m_size; }

    /**
     * Returns the size of the currently allocated storage array.
     *
     * ```
     * auto vec = Vector<char>(10);
     * assert_eq(10, vec.capacity());
     * ```
     */
    size_t capacity() const { return m_capacity; }

    /**
     * Return a pointer to the underlying storage array.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * char *ary = vec.data();
     * assert_eq('b', ary[1]);
     * ```
     */
    T *data() { return m_data; }

    /**
     * Fill the given range with a filler value.
     *
     * The 'to' index is exclusive.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * vec.fill(1, 3, 'z');
     * assert_eq('a', vec[0]);
     * assert_eq('z', vec[1]);
     * assert_eq('z', vec[2]);
     * ```
     *
     * This method aborts if the 'to' index is past the end.
     *
     * ```should_abort
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * vec.fill(0, 10, 'z');
     * ```
     */
    void fill(size_t from, size_t to_exclusive, T filler) {
        assert(to_exclusive <= m_size);
        for (size_t i = from; i < to_exclusive; i++) {
            m_data[i] = filler;
        }
    }

    /**
     * Shrink the vector.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * vec.set_size(2);
     * assert_eq(2, vec.size());
     * assert_eq('a', vec[0]);
     * assert_eq('b', vec[1]);
     * ```
     *
     * This method aborts if the new size is larger.
     *
     * ```should_abort
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * vec.set_size(10);
     * ```
     */
    void set_size(size_t new_size) {
        assert(new_size <= m_size);
        grow(new_size);
        m_size = new_size;
    }

    /**
     * Grow or shrink the vector.
     * New slots will be filled with the param `filler`.
     *
     * ```
     * auto vec = Vector<char> { 'a', 'b', 'c' };
     * vec.set_size(10, 'z'); // grow
     * assert_eq('z', vec[9]);
     *
     * vec.set_size(2, 'x'); // shrink
     * assert_eq(2, vec.size());
     * assert_eq('a', vec[0]);
     * assert_eq('b', vec[1]);
     * ```
     */
    void set_size(size_t new_size, T filler) {
        grow(new_size);
        size_t old_size = m_size;
        m_size = new_size;
        fill(old_size, new_size, filler);
    }

    /**
     * Grow the capacity (allocated memory) of the vector.
     *
     * ```
     * auto vec = Vector<char> {};
     * vec.set_capacity(100);
     * assert_eq(100, vec.capacity());
     * ```
     */
    void set_capacity(size_t new_size) {
        grow_at_least(new_size);
    }

    class iterator {
    public:
        iterator(const Vector<T> *vector, size_t index)
            : m_vector { vector }
            , m_index { index } { }

        iterator operator++() {
            m_index++;
            return *this;
        }

        iterator operator++(int) {
            iterator i = *this;
            m_index++;
            return i;
        }

        T &operator*() { return m_vector->m_data[m_index]; }
        T *operator->() { return m_vector->m_data[m_index]; }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_vector == i2.m_vector && i1.m_index == i2.m_index;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_vector != i2.m_vector || i1.m_index != i2.m_index;
        }

    private:
        const Vector<T> *m_vector;
        size_t m_index { 0 };
    };

    /**
     * Returns an iterator over the vector.
     *
     * ```
     * auto vec1 = Vector<char> { 'a', 'b', 'c' };
     * auto vec2 = Vector<char> {};
     * for (auto c : vec1) {
     *     vec2.push(c);
     * }
     * assert_eq('a', vec2[0]);
     * assert_eq('b', vec2[1]);
     * assert_eq('c', vec2[2]);
     * ```
     */
    iterator begin() {
        return iterator { this, 0 };
    }

    iterator begin() const {
        return iterator { this, 0 };
    }

    iterator end() {
        return iterator { this, m_size };
    }

    iterator end() const {
        return iterator { this, m_size };
    }

    /**
     * Sorts the vector using the given lambda or callable type.
     *
     * ```
     * auto vec = Vector<char> { 'b', 'c', 'a' };
     * vec.sort([](char a, char b) { return a - b < 0; });
     * assert_eq('a', vec[0]);
     * assert_eq('b', vec[1]);
     * assert_eq('c', vec[2]);
     * ```
     */
    template <typename F>
    void sort(F cmp) {
        quicksort(0, m_size - 1, cmp);
    }

private:
    Vector(size_t size, size_t capacity, T *data)
        : m_size(size)
        , m_capacity(capacity)
        , m_data(data) { }

    static T *array_of_size(size_t size) {
        if constexpr (std::is_trivially_copyable<T>::value)
            return reinterpret_cast<T *>(malloc(size * sizeof(T)));
        else
            return new T[size] {};
    }

    void grow(size_t capacity) {
        if (m_capacity >= capacity)
            return;
        if constexpr (std::is_trivially_copyable<T>::value) {
            m_data = static_cast<T *>(realloc(m_data, capacity * sizeof(T)));
        } else {
            auto old_data = m_data;
            m_data = new T[capacity] {};
            for (size_t i = 0; i < m_size; ++i)
                m_data[i] = old_data[i];
            delete[] old_data;
        }
        m_capacity = capacity;
    }

    void grow_at_least(size_t min_capacity) {
        if (m_capacity >= min_capacity) {
            return;
        }
        if (m_capacity > 0 && min_capacity <= m_capacity * VECTOR_GROW_FACTOR) {
            grow(m_capacity * VECTOR_GROW_FACTOR);
        } else {
            grow(min_capacity);
        }
    }

    template <typename F>
    void quicksort(int start, int end, F cmp) {
        if (start >= end) return;
        int p_index = quicksort_partition(start, end, cmp);
        quicksort(start, p_index - 1, cmp);
        quicksort(p_index + 1, end, cmp);
    }

    template <typename F>
    int quicksort_partition(int start, int end, F cmp) {
        T pivot = m_data[end];
        int p_index = start;
        T temp;

        for (int i = start; i < end; i++) {
            if (cmp(m_data[i], pivot)) {
                temp = m_data[i];
                m_data[i] = m_data[p_index];
                m_data[p_index] = temp;
                p_index++;
            }
        }
        temp = m_data[end];
        m_data[end] = m_data[p_index];
        m_data[p_index] = temp;
        return p_index;
    }

    void copy_data(T *dest, T *src, size_t size) {
        if constexpr (std::is_trivially_copyable<T>::value) {
            memcpy(dest, src, sizeof(T) * size);
        } else {
            for (size_t i = 0; i < size; ++i)
                dest[i] = src[i];
        }
    }

    void delete_memory() {
        if constexpr (std::is_trivially_copyable<T>::value)
            free(m_data);
        else
            delete[] m_data;
    }

    size_t m_size { 0 };
    size_t m_capacity { 0 };
    T *m_data { nullptr };
};

}
