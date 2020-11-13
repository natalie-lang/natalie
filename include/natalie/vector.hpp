#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "natalie/gc.hpp"

#define NAT_VECTOR_GROW_FACTOR 2
#define NAT_VECTOR_MIN_CAPACITY 10

namespace Natalie {

template <typename T>
struct Vector : public gc {
    Vector()
        : m_capacity { NAT_VECTOR_MIN_CAPACITY }
        , m_data { static_cast<T *>(GC_MALLOC(sizeof(T) * NAT_VECTOR_MIN_CAPACITY)) } {
        memset(m_data, 0, sizeof(T) * NAT_VECTOR_MIN_CAPACITY);
    }

    Vector(ssize_t initial_capacity, T filler)
        : m_size { initial_capacity }
        , m_capacity { initial_capacity }
        , m_data { static_cast<T *>(GC_MALLOC(sizeof(T) * initial_capacity)) } {
        fill(0, initial_capacity, filler);
    }

    Vector(const Vector &other)
        : m_size { other.m_size }
        , m_capacity { other.m_size }
        , m_data { static_cast<T *>(GC_MALLOC(sizeof(T) * m_size)) } {
        memcpy(m_data, other.m_data, sizeof(T) * m_size);
    }

    Vector slice(ssize_t offset, ssize_t count = -1) {
        if (count == -1 && m_size >= offset)
            count = m_size - offset;
        auto size = count;
        auto capacity = size;
        T *data = nullptr;
        if (offset >= 0 && count >= 0 && m_size >= offset + count) {
            if constexpr (NAT_VECTOR_GROW_FACTOR == 2) {
                --capacity;
                for (auto i = 0; i < 64; i += 4)
                    capacity |= capacity >> i;
                ++capacity;
            } else {
                capacity /= NAT_VECTOR_GROW_FACTOR;
                ++capacity;
                capacity *= NAT_VECTOR_GROW_FACTOR;
            }
            data = static_cast<T *>(GC_MALLOC(sizeof(T) * capacity));
            memcpy(data, m_data + offset, sizeof(T) * size);
        } else {
            // FIXME: We should probably say something here.
            size = 0;
            capacity = 0;
            data = nullptr;
        }

        return { size, capacity, data };
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

    ~Vector() {
        if (m_data)
            GC_FREE(m_data);
    }

    T &operator[](ssize_t index) const {
        return m_data[index];
    }

    void push(T val) {
        ssize_t len = m_size;
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
        for (ssize_t i = m_size - 1; i > 0; i--) {
            m_data[i] = m_data[i - 1];
        }
        m_data[0] = val;
    }

    ssize_t is_empty() const { return m_size == 0; }
    ssize_t size() const { return m_size; }
    ssize_t capacity() const { return m_capacity; }
    T *data() { return m_data; }

    void fill(ssize_t from, ssize_t to_exclusive, T filler) {
        assert(from >= 0);
        assert(to_exclusive <= m_size);
        for (ssize_t i = from; i < to_exclusive; i++) {
            m_data[i] = filler;
        }
    }

    void set_size(ssize_t new_size) {
        assert(new_size <= m_size);
        grow(new_size);
        m_size = new_size;
    }

    void set_size(ssize_t new_size, T filler) {
        grow(new_size);
        ssize_t old_size = m_size;
        m_size = new_size;
        fill(old_size, new_size, filler);
    }

    void set_capacity(ssize_t new_size) {
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
    Vector(ssize_t size, ssize_t capacity, T *data)
        : m_size(size)
        , m_capacity(capacity)
        , m_data(data) { }

    void grow(ssize_t capacity) {
        m_data = static_cast<T *>(GC_REALLOC(m_data, sizeof(T) * capacity));
        m_capacity = capacity;
    }

    void grow_at_least(ssize_t min_capacity) {
        if (m_capacity >= min_capacity) {
            return;
        }
        if (m_capacity > 0 && min_capacity <= m_capacity * NAT_VECTOR_GROW_FACTOR) {
            grow(m_capacity * NAT_VECTOR_GROW_FACTOR);
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

    ssize_t m_size { 0 };
    ssize_t m_capacity { 0 };
    T *m_data { nullptr };
};

}
