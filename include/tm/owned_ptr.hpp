#pragma once

#include <assert.h>

namespace TM {

template <typename T>
class OwnedPtr {
public:
    /**
     * Constructs a null OwnedPtr.
     *
     * ```
     * auto ptr = OwnedPtr<Thing>();
     * assert(!ptr);
     * ```
     */
    OwnedPtr()
        : m_ptr { nullptr } { }

    /**
     * Constructs an OwnedPtr with the given raw pointer.
     *
     * ```
     * auto ptr = OwnedPtr<Thing>(new Thing(1));
     * assert(ptr);
     * ```
     *
     * Alternative syntax:
     *
     * ```
     * OwnedPtr<Thing> ptr = new Thing(1);
     * assert(ptr);
     * ```
     */
    OwnedPtr(T *ptr)
        : m_ptr { ptr } { }

    ~OwnedPtr() {
        delete m_ptr;
    }

    OwnedPtr(const OwnedPtr &other) = delete;

    /**
     * Overwrites this OwnedPtr with a new raw pointer,
     * destroying our existing held object, if any.
     *
     * ```
     * auto obj1 = new Thing(1);
     * auto ptr = OwnedPtr<Thing>(obj1);
     * ptr = new Thing(2);
     * // obj1 has been destroyed
     * ```
     */
    OwnedPtr<T> &operator=(T *ptr) {
        if (ptr != m_ptr)
            delete m_ptr;
        m_ptr = ptr;
        return *this;
    }

    OwnedPtr<T> &operator=(const OwnedPtr<T> &other) = delete;

    /**
     * Returns true if this OwnedPtr is not null.
     *
     * ```
     * auto ptr1 = OwnedPtr<Thing>(new Thing(1));
     * auto ptr2 = OwnedPtr<Thing>();
     * assert(ptr1);
     * assert(!ptr2);
     * ```
     */
    operator bool() const {
        return !!m_ptr;
    }

    /**
     * Returns a reference to the underlying raw pointer.
     *
     * ```
     * auto raw = new Thing(1);
     * auto ptr = OwnedPtr<Thing>(raw);
     * assert_eq(*raw, *ptr);
     * ```
     *
     * This method aborts if the pointer is null.
     *
     * ```should_abort
     * auto ptr = OwnedPtr<Thing>();
     * *ptr;
     * ```
     */
    const T &operator*() const {
        assert(m_ptr);
        return *m_ptr;
    }

    /**
     * Returns a reference to the underlying raw pointer.
     *
     * ```
     * auto raw = new Thing(1);
     * auto ptr = OwnedPtr<Thing>(raw);
     * assert_eq(*raw, ptr.ref());
     * ```
     *
     * This method aborts if the pointer is null.
     *
     * ```should_abort
     * auto ptr = OwnedPtr<Thing>();
     * ptr.ref();
     * ```
     */
    const T &ref() const {
        assert(m_ptr);
        return *m_ptr;
    }

    /**
     * Dereferences the underlying raw pointer for
     * chained member reference.
     *
     * ```
     * auto raw = new Thing(1);
     * auto ptr = OwnedPtr<Thing>(raw);
     * assert_eq(1, ptr->value());
     * ```
     *
     * This method aborts if the pointer is null.
     *
     * ```should_abort
     * auto ptr = OwnedPtr<Thing>();
     * ptr->value();
     * ```
     */
    T *operator->() const {
        assert(m_ptr);
        return m_ptr;
    }

private:
    T *m_ptr;
};

}
