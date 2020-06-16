#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "natalie/gc.hpp"

#define NAT_VECTOR_GROW_FACTOR 2

namespace Natalie {

template <typename T>
struct Vector : public gc {
    Vector() { }

    Vector(ssize_t initial_capacity)
        : m_size { initial_capacity }
        , m_capacity { initial_capacity }
        , m_data { static_cast<T *>(malloc(sizeof(T) * initial_capacity)) } { }

    Vector(const Vector &other)
        : m_size { other.m_size }
        , m_capacity { other.m_size }
        , m_data { static_cast<T *>(malloc(sizeof(T) * m_size)) } {
        memcpy(m_data, other.m_data, sizeof(T) * m_size);
    }

    ~Vector() {
        free(m_data);
    }

    T &operator[](ssize_t index) {
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

    ssize_t is_empty() { return m_size == 0; }
    ssize_t size() { return m_size; }
    ssize_t capacity() { return m_capacity; }
    T *data() { return m_data; }

    void set_size(ssize_t new_size) {
        grow(new_size);
        m_size = new_size;
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
    void grow(ssize_t capacity) {
        m_data = static_cast<T *>(realloc(m_data, sizeof(T) * capacity));
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
