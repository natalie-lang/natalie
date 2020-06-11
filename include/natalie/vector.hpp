#pragma once

#include <algorithm>
#include <iterator>
#include <stdlib.h>

#define NAT_VECTOR_GROW_FACTOR 2

namespace Natalie {

template <typename T>
struct MyVector {
    MyVector() { }

    MyVector(ssize_t initial_capacity)
        : m_size { initial_capacity }
        , m_capacity { initial_capacity }
        , m_data { new T[initial_capacity] {} } { }

    ~MyVector() {
        delete[] m_data;
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

    ssize_t size() { return m_size; }
    ssize_t capacity() { return m_capacity; }
    T *data() { return m_data; }

    void set_size(ssize_t new_size) {
        grow(new_size);
        m_size = new_size;
    }

    void sort(std::function<bool(T, T)> cmp) {
        std::sort(m_data, m_data + m_size, cmp);
    }

    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        iterator(T *ptr)
            : m_ptr { ptr } { }

        /*
        iterator(const iterator &other)
            : m_ptr { other.m_ptr } { }

        iterator operator=(const iterator &other) {
            m_ptr = other.m_ptr;
            return *this;
        }

        iterator operator++() {
            m_ptr++;
            return *this;
        }


        T &operator*() { return *m_ptr; }
        T *operator->() { return m_ptr; }
        bool operator==(const T &rhs) { return m_ptr == rhs.m_ptr; }
        bool operator!=(const T &rhs) { return m_ptr != rhs.m_ptr; }
        //bool operator==(const T *rhs) { return m_ptr == rhs->m_ptr; }
        //bool operator!=(const T *rhs) { return m_ptr != rhs->m_ptr; }
        //bool operator==(const iterator &i) { return m_ptr == i.m_ptr; }
        bool operator!=(const iterator &i) { return m_ptr != i.m_ptr; }
        */

        T &operator*() { return *m_ptr; }

        iterator operator++() {
            m_ptr++;
            return *this;
        }

        iterator operator++(int _) {
            iterator i = *this;
            m_ptr++;
            return i;
        }

        value_type operator*() const { return *m_ptr; }

        pointer operator->() const { return m_ptr; }

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

private:
    void grow(ssize_t capacity) {
        T *new_data = new T[capacity] {};
        std::copy_n(m_data, std::min(capacity, m_capacity), new_data);
        delete[] m_data;
        m_data = new_data;
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

    ssize_t m_size { 0 };
    ssize_t m_capacity { 0 };
    T *m_data { nullptr };
};

}
