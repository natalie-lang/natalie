#pragma once

#include <assert.h>
#include <stdio.h>

namespace TM {

class Counter {
public:
    Counter(unsigned int count)
        : m_count { count } { }

    void increment() { m_count++; }
    void decrement() { m_count--; }

    bool operator==(unsigned int other) const {
        return m_count == other;
    }

    unsigned int count() const { return m_count; }

private:
    unsigned int m_count;
};

template <typename T>
class SharedPtr {
public:
    SharedPtr()
        : m_ptr { nullptr }
        , m_count { nullptr } { }

    SharedPtr(T *ptr)
        : m_ptr { ptr }
        , m_count { new Counter(1) } {
        assert(m_ptr);
    }

    ~SharedPtr() {
        destroy();
    }

    SharedPtr(const SharedPtr &other)
        : m_ptr { other.m_ptr }
        , m_count { other.m_count } {
        assert(valid());
        if (m_count)
            m_count->increment();
    }

    SharedPtr<T> &operator=(const SharedPtr<T> &other) {
        destroy();
        m_ptr = other.m_ptr;
        m_count = other.m_count;
        assert(valid());
        if (m_count)
            m_count->increment();
        return *this;
    }

    operator bool() {
        return !!m_ptr;
    }

    T &operator*() const {
        assert(m_ptr);
        return *m_ptr;
    }

    T &ref() const {
        assert(m_ptr);
        return *m_ptr;
    }

    T *operator->() const {
        assert(m_ptr);
        return m_ptr;
    }

    unsigned int count() {
        if (!m_count)
            return 0;
        return m_count->count();
    }

private:
    bool valid() {
        return (m_ptr && m_count) || (!m_ptr && !m_count);
    }

    void destroy() {
        if (m_ptr == nullptr) {
            delete m_count;
            return;
        }
        assert(m_count->count() > 0);
        m_count->decrement();
        if (m_count->count() == 0) {
            delete m_ptr;
            delete m_count;
        }
    }

    T *m_ptr;
    Counter *m_count;
};

}
