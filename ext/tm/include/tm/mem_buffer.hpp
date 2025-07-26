#pragma once

#include <cstddef>

namespace TM {

template <class T>
class MemBuffer {
public:
    /**
     * Create a new memory buffer. The buffer managed the memory
     *
     * ```
     * MemBuffer<int> buf { 5 };
     * assert_eq(5, buf.size());
     * ```
     */
    explicit MemBuffer(const size_t size)
        : m_buffer { new T[size] }
        , m_size { size } { }

    MemBuffer(const MemBuffer<T> &) = delete;

    /**
     * Create a membuffer by moving from another MemBuffer
     *
     * ```
     * MemBuffer<int> buf { 5 };
     * MemBuffer<int> buf2 { std::move(buf) };
     * assert_eq(0, buf.size());
     * assert_eq(5, buf2.size());
     * ```
     */
    MemBuffer(MemBuffer<T> &&other) {
        m_buffer = other.m_buffer;
        m_size = other.m_size;
        other.m_buffer = nullptr;
        other.m_size = 0;
    }

    MemBuffer &operator=(const MemBuffer &) = delete;

    /**
     * Create a membuffer by moving from another MemBuffer
     *
     * ```
     * MemBuffer<int> buf { 5 };
     * MemBuffer<int> buf2 = std::move(buf);
     * assert_eq(0, buf.size());
     * assert_eq(5, buf2.size());
     * ```
     */
    MemBuffer &operator=(MemBuffer<T> &&other) {
        m_buffer = other.m_buffer;
        m_size = other.m_size;
        other.m_buffer = nullptr;
        other.m_size = 0;
    }

    ~MemBuffer() {
        delete[] m_buffer;
    }

    /**
     * Get the size of a MemBuffer
     *
     * ```
     * MemBuffer<int> buf { 5 };
     * assert_eq(5, buf.size());
     * ```
     */
    size_t size() const {
        return m_size;
    }

    operator T *() & = delete;

    /**
     * Get the raw memory buffer. This memory is no longer managed by the MemBuffer object.
     *
     * ```
     * MemBuffer<int> buf { 5 };
     * int* raw_buf { std::move(buf) };
     * delete[] raw_buf;
     * ```
     */
    operator T *() && {
        T *ptr = m_buffer;
        m_buffer = nullptr;
        m_size = 0;
        return ptr;
    }

private:
    T *m_buffer;
    size_t m_size;
};

}
