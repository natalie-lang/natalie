#pragma once

#include <assert.h>
#include <initializer_list>
#include <stddef.h>

#define TM_MIN(a, b) (a < b ? a : b)

namespace TM {

const int VECTOR_GROW_FACTOR = 2;
const int VECTOR_MIN_CAPACITY = 10;

template <typename T>
class Vector {
public:
    Vector()
        : m_capacity { VECTOR_MIN_CAPACITY }
        , m_data { new T[VECTOR_MIN_CAPACITY] {} } { }

    Vector(size_t initial_capacity)
        : m_size { initial_capacity }
        , m_capacity { initial_capacity }
        , m_data { new T[initial_capacity] {} } { }

    Vector(size_t initial_capacity, T filler)
        : Vector { initial_capacity } {
        fill(0, initial_capacity, filler);
    }

    Vector(std::initializer_list<T> list)
        : Vector { list.size() } {
        for (auto v : list) {
            push(v);
        }
    }

    Vector(const Vector &other)
        : m_size { other.m_size }
        , m_capacity { other.m_size }
        , m_data { new T[m_size] {} } {
        memcpy(m_data, other.m_data, sizeof(T) * m_size);
    }

    ~Vector() {
        delete[] m_data;
    }

    Vector slice(size_t offset, size_t count = 0) {
        if (count == 0 || offset + count > m_size) {
            count = m_size - offset;
        }
        if (offset >= m_size || count == 0) {
            return {};
        }
        T *data = new T[count] {};
        memcpy(data, m_data + offset, sizeof(T) * count);
        return { count, count, data };
    }

    Vector &operator=(Vector &&other) {
        m_size = other.m_size;
        m_capacity = other.m_size;
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
        if (m_size >= m_capacity) {
            grow_at_least(m_size + 1);
        }
        m_size++;
        for (size_t i = m_size - 1; i > 0; i--) {
            m_data[i] = m_data[i - 1];
        }
        m_data[0] = val;
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
        iterator(T *ptr)
            : m_ptr { ptr } { }

        iterator operator++() {
            m_ptr++;
            return *this;
        }

        iterator operator++(int _) {
            iterator i = *this;
            m_ptr++;
            return i;
        }

        T &operator*() { return *m_ptr; }
        T *operator->() { return m_ptr; }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_ptr == i2.m_ptr;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_ptr != i2.m_ptr;
        }

    private:
        T *m_ptr;
    };

    iterator begin() {
        return iterator { m_data };
    }

    iterator end() {
        return iterator { m_data + m_size };
    }

    struct SortComparator {
        void *data;
        bool (*cmp)(void *, T, T);
    };

    void sort(SortComparator cmp) {
        quicksort(0, m_size - 1, cmp);
    }

private:
    Vector(size_t size, size_t capacity, T *data)
        : m_size(size)
        , m_capacity(capacity)
        , m_data(data) { }

    void grow(size_t capacity) {
        auto old_data = m_data;
        m_data = new T[capacity] {};
        memcpy(m_data, old_data, sizeof(T) * TM_MIN(capacity, m_capacity));
        delete[] old_data;
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

    void quicksort(int start, int end, SortComparator cmp) {
        if (start >= end) return;
        int p_index = quicksort_partition(start, end, cmp);
        quicksort(start, p_index - 1, cmp);
        quicksort(p_index + 1, end, cmp);
    }

    int quicksort_partition(int start, int end, SortComparator cmp) {
        T pivot = m_data[end];
        int p_index = start;
        T temp;

        for (int i = start; i < end; i++) {
            if (cmp.cmp(cmp.data, m_data[i], pivot)) {
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

    size_t m_size { 0 };
    size_t m_capacity { 0 };
    T *m_data { nullptr };
};

}
