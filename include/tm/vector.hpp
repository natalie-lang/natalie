#pragma once

#include <algorithm>
#include <assert.h>
#include <initializer_list>
#include <stddef.h>
#include <type_traits>

namespace TM {

const int VECTOR_GROW_FACTOR = 2;
const int VECTOR_MIN_CAPACITY = 10;

template <typename T>
class Vector {
public:
    Vector()
        : m_capacity { VECTOR_MIN_CAPACITY }
        , m_data { array_of_size(VECTOR_MIN_CAPACITY) } { }

    Vector(size_t initial_capacity)
        : m_capacity { initial_capacity }
        , m_data { array_of_size(initial_capacity) } { }

    Vector(size_t initial_capacity, T filler)
        : m_size { initial_capacity }
        , m_capacity { initial_capacity }
        , m_data { array_of_size(initial_capacity) } {
        fill(0, initial_capacity, filler);
    }

    Vector(std::initializer_list<T> list)
        : m_capacity { list.size() }
        , m_data { array_of_size(list.size()) } {
        for (auto v : list) {
            push(v);
        }
    }

    Vector(const Vector &other)
        : m_size { other.m_size }
        , m_capacity { other.m_size }
        , m_data { array_of_size(other.m_size) } {
        copy_data(m_data, other.m_data, other.m_size);
    }

    ~Vector() {
        delete_memory();
    }

    void clear() { m_size = 0; }

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

    Vector &operator=(Vector &other) {
        clear();
        grow_at_least(other.m_size);
        copy_data(m_data, other.m_data, other.m_size);
        m_size = other.m_size;
        return *this;
    }

    void copy_data(T *dest, T *src, size_t size) {
        if constexpr (std::is_trivially_copyable<T>::value) {
            memcpy(dest, src, sizeof(T) * size);
        } else {
            for (size_t i = 0; i < size; ++i)
                dest[i] = src[i];
        }
    }

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

    T &operator[](size_t index) const {
        return m_data[index];
    }

    T &at(size_t index) const {
        assert(index < m_size);
        return m_data[index];
    }

    T first() {
        assert(m_size != 0);
        return m_data[0];
    }

    T last() {
        assert(m_size != 0);
        return m_data[m_size - 1];
    }

    T pop() {
        assert(m_size != 0);
        return m_data[--m_size];
    }

    void push(T val) {
        size_t len = m_size;
        if (m_size >= m_capacity) {
            grow_at_least(m_size + 1);
        }
        m_size++;
        m_data[len] = val;
    }

    void push_front(T val) {
        insert(0, val);
    }

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
            for (size_t i = m_size - 1; i > index; --i) {
                m_data[i + 1] = m_data[i];
            }
            m_data[index + 1] = m_data[index];
        }

        m_data[index] = val;
        m_size++;
    }

    T pop_front() {
        T val = m_data[0];
        remove(0);
        return val;
    }

    void remove(size_t index) {
        assert(m_size > index);

        --m_size;

        if (index == m_size)
            return;

        memmove(m_data + index, m_data + index + 1, (m_size - index) * sizeof(T));
    }

    bool is_empty() const { return m_size == 0; }

    size_t size() const { return m_size; }
    size_t capacity() const { return m_capacity; }
    T *data() { return m_data; }

    void fill(size_t from, size_t to_exclusive, T filler) {
        assert(to_exclusive <= m_size);
        for (size_t i = from; i < to_exclusive; i++) {
            m_data[i] = filler;
        }
    }

    void set_size(size_t new_size) {
        assert(new_size <= m_size);
        grow(new_size);
        m_size = new_size;
    }

    void set_size(size_t new_size, T filler) {
        grow(new_size);
        size_t old_size = m_size;
        m_size = new_size;
        fill(old_size, new_size, filler);
    }

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
