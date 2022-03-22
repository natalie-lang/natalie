#pragma once

#include <assert.h>

namespace TM {

template <typename T>
class OwnedPtr {
public:
    OwnedPtr()
        : m_ptr { nullptr } { }

    OwnedPtr(T *ptr)
        : m_ptr { ptr } { }

    ~OwnedPtr() {
        delete m_ptr;
    }

    OwnedPtr(const OwnedPtr &other) = delete;

    OwnedPtr<T> &operator=(T *ptr) {
        if (ptr != m_ptr)
            delete m_ptr;
        m_ptr = ptr;
        return *this;
    }

    OwnedPtr<T> &operator=(const OwnedPtr<T> &other) = delete;

    operator bool() const {
        return !!m_ptr;
    }

    const T &operator*() const {
        assert(m_ptr);
        return *m_ptr;
    }

    const T &ref() const {
        assert(m_ptr);
        return *m_ptr;
    }

    T *operator->() const {
        assert(m_ptr);
        return m_ptr;
    }

private:
    T *m_ptr;
};

}
