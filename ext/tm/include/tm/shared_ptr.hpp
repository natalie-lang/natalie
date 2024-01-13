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
    /**
     * Constructs a null SharedPtr.
     *
     * ```
     * auto ptr = SharedPtr<Thing>();
     * assert_not(ptr);
     * ```
     */
    SharedPtr()
        : m_ptr { nullptr }
        , m_count { nullptr } { }

    /**
     * Constructs a SharedPtr with the given raw pointer.
     *
     * ```
     * auto ptr = SharedPtr<Thing>(new Thing(1));
     * assert(ptr);
     * ```
     *
     * Alternative syntax:
     *
     * ```
     * SharedPtr<Thing> ptr = new Thing(1);
     * assert(ptr);
     * ```
     */
    SharedPtr(T *ptr)
        : m_ptr { ptr }
        , m_count { new Counter(1) } {
        assert(m_ptr);
    }

    SharedPtr(T *ptr, Counter *counter)
        : m_ptr { ptr }
        , m_count { counter } {
        assert(m_ptr);
        assert(m_count);
        m_count->increment();
    }

    ~SharedPtr() {
        destroy();
    }

    /**
     * Copies the given SharedPtr, incrementing the internal
     * reference counter.
     *
     * ```
     * auto ptr1 = SharedPtr<Thing>(new Thing(1));
     * auto ptr2 = SharedPtr<Thing>(ptr1);
     * assert_eq(2, ptr1.count());
     * assert_eq(2, ptr2.count());
     * ```
     */
    SharedPtr(const SharedPtr &other)
        : m_ptr { other.m_ptr }
        , m_count { other.m_count } {
        assert(valid());
        if (m_count)
            m_count->increment();
    }

    /**
     * Overwrites this SharedPtr with the given one,
     * decrementing the ref count for our old pointer,
     * and incrementing the ref counter for our new pointer.
     *
     * ```
     * auto ptr1 = SharedPtr<Thing>(new Thing(1));
     * auto ptr2 = SharedPtr<Thing>(new Thing(2));
     * auto ptr3 = SharedPtr<Thing>(ptr2);
     * assert_eq(1, ptr1.count());
     * assert_eq(2, ptr3.count());
     *
     * ptr2 = ptr1;
     * assert_eq(2, ptr1.count());
     * assert_eq(1, ptr3.count());
     * ```
     */
    SharedPtr<T> &operator=(const SharedPtr<T> &other) {
        destroy();
        m_ptr = other.m_ptr;
        m_count = other.m_count;
        assert(valid());
        if (m_count)
            m_count->increment();
        return *this;
    }

    /**
     * Returns true if this SharedPtr is not null.
     *
     * ```
     * auto ptr1 = SharedPtr<Thing>(new Thing(1));
     * auto ptr2 = SharedPtr<Thing>();
     * assert(ptr1);
     * assert_not(ptr2);
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
     * auto ptr = SharedPtr<Thing>(raw);
     * assert_eq(*raw, *ptr);
     * ```
     *
     * This method aborts if the pointer is null.
     *
     * ```should_abort
     * auto ptr = SharedPtr<Thing>();
     * *ptr;
     * ```
     */
    T &operator*() const {
        assert(m_ptr);
        return *m_ptr;
    }

    /**
     * Returns a reference to the underlying raw pointer.
     *
     * ```
     * auto raw = new Thing(1);
     * auto ptr = SharedPtr<Thing>(raw);
     * assert_eq(*raw, ptr.ref());
     * ```
     *
     * This method aborts if the pointer is null.
     *
     * ```should_abort
     * auto ptr = SharedPtr<Thing>();
     * ptr.ref();
     * ```
     */
    T &ref() const {
        assert(m_ptr);
        return *m_ptr;
    }

    /**
     * Dereferences the underlying raw pointer for
     * chained member reference.
     *
     * ```
     * auto raw = new Thing(1);
     * auto ptr = SharedPtr<Thing>(raw);
     * assert_eq(1, ptr->value());
     * ```
     *
     * This method aborts if the pointer is null.
     *
     * ```should_abort
     * auto ptr = SharedPtr<Thing>();
     * ptr->value();
     * ```
     */
    T *operator->() const {
        assert(m_ptr);
        return m_ptr;
    }

    /**
     * Returns the current count of outstanding copies of this SharedPtr.
     *
     * ```
     * auto ptr1 = SharedPtr<Thing>(new Thing(1));
     * assert_eq(1, ptr1.count());
     * auto ptr2 = ptr1;
     * auto ptr3 = new SharedPtr<Thing>(ptr1);
     * assert_eq(3, ptr1.count());
     * delete ptr3;
     * assert_eq(2, ptr1.count());
     * ```
     *
     * If this is a null SharedPtr, returns 0.
     *
     * ```
     * auto ptr = SharedPtr<Thing>();
     * assert_eq(0, ptr.count());
     * ```
     */
    unsigned int count() const {
        if (!m_count)
            return 0;
        return m_count->count();
    }

    /**
     * Returns a new SharedPtr with the underlying pointer
     * statically cast as the templated type.
     *
     * TODO: example
     */
    template <typename To>
    SharedPtr<To> static_cast_as() const {
        if (!m_ptr)
            return {};
        return SharedPtr<To> { static_cast<To *>(m_ptr), m_count };
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
