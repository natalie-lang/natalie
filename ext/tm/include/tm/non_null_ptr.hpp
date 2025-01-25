#pragma once

#include <assert.h>

namespace TM {

template <typename T>
class NonNullPtr {
public:
    NonNullPtr() = delete;

    /**
     * Creates a new NonNullPtr from a raw pointer.
     *
     * ```
     * auto raw = new int(42);
     * NonNullPtr<int> ptr(raw);
     * delete raw;
     * ```
     *
     * ```should_abort
     * NonNullPtr<int> ptr(nullptr);
     * ```
     */
    NonNullPtr(T *ptr)
        : m_ptr(ptr) {
        assert(m_ptr);
    }

    NonNullPtr(const NonNullPtr<T> &other)
        : m_ptr { other.m_ptr } {
        assert(m_ptr);
    }

    NonNullPtr<T> &operator=(const NonNullPtr<T> &other) {
        if (this != &other) {
            m_ptr = other.m_ptr;
            assert(m_ptr);
        }
        return *this;
    }

    /**
     * Overwrites the raw pointer with the one given.
     *
     * ```
     * auto raw1 = new int(42);
     * auto raw2 = new int(43);
     * NonNullPtr<int> ptr(raw1);
     * ptr = raw2;
     * delete raw1;
     * delete raw2;
     * ```
     *
     * ```should_abort
     * auto raw = new int(42);
     * NonNullPtr<int> ptr(raw);
     * ptr = nullptr;
     * ```
     */
    NonNullPtr<T> &operator=(T *ptr) {
        assert(ptr);
        m_ptr = ptr;
        return *this;
    }

    T *&operator*() {
        return *m_ptr;
    }

    const T *&operator*() const {
        return *m_ptr;
    }

    T *operator->() {
        return m_ptr;
    }

    const T *operator->() const {
        return m_ptr;
    }

    bool operator==(const NonNullPtr<T> &other) const {
        return m_ptr == other.m_ptr;
    }

    bool operator!=(const NonNullPtr<T> &other) const {
        return m_ptr != other.m_ptr;
    }

    bool operator==(const T *other) const {
        return m_ptr == other;
    }

    bool operator!=(const T *other) const {
        return m_ptr != other;
    }

    operator bool() const { return true; }
    bool operator!() const { return false; }

    T *ptr() const { return m_ptr; }

private:
    T *m_ptr;
};

}
