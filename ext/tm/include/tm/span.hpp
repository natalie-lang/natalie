#pragma once

#include <cassert>
#include <cstddef>

namespace TM {

template <typename T>
class Span {
public:
    /**
     * Construct a span over an existing list
     *
     * ```
     * int list[] = { 1, 2, 3, 4, 5 };
     * Span span { list, 2 };
     * assert_eq(2, span.size());
     * ```
     */
    Span(const T *data, const size_t size)
        : m_data { data }
        , m_size { size } {
        assert(m_size > 0);
    }

    Span(const Span &) = default;
    Span(Span &&) = default;
    Span &operator=(const Span &) = default;
    Span &operator=(Span &&) = default;

    /**
     * Returns the number of items in the span.
     *
     * ```
     * const char list[] = { 'a', 'b', 'c', 'd', 'e' };
     * Span span { list, 2 };
     * assert_eq(2, span.size());
     * ```
     */
    size_t size() const { return m_size; }

    /**
     * Returns a reference to the value at the given index.
     *
     * ```
     * const char list[] = { 'a', 'b', 'c' };
     * const Span span { list, 3 };
     * assert_eq('b', span[1]);
     * ```
     *
     * WARNING: This method does *not* check that the given
     * index is within the bounds of the span!
     */
    const T &operator[](const size_t index) const {
        return m_data[index];
    }

    /**
     * Returns a reference to the value at the given index.
     *
     * ```
     * const char list[] = { 'a', 'b', 'c', 'd' };
     * Span span { list + 1, 3 };
     * assert_eq('c', span.at(1));
     * ```
     *
     * This method aborts if the index is past the end.
     *
     * ```should_abort
     * const char list[] = { 'a', 'b', 'c' };
     * Span span { list, 1 };
     * span.at(1);
     * ```
     */
    const T &at(const size_t index) const {
        assert(index < m_size);
        return m_data[index];
    }

    /**
     * Returns a new span from the given offset and count.
     *
     * ```
     * const char list[] = { 'a', 'b', 'c', 'd', 'e' };
     * Span span1 { list, 5 };
     * auto span2 = span1.slice(2, 2);
     * assert_eq(2, span2.size());
     * assert_eq('c', span2[0]);
     * assert_eq('d', span2[1]);
     * ```
     *
     * If `count` is not specified, then the returned vector
     * will include all items from the index to the end.
     *
     * ```
     * const char list[] = { 'a', 'b', 'c', 'd', 'e' };
     * Span span1 { list, 5 };
     * auto span2 = span1.slice(2);
     * assert_eq(3, span2.size());
     * assert_eq('c', span2[0]);
     * assert_eq('d', span2[1]);
     * assert_eq('e', span2[2]);
     * ```
     *
     * The size cannot exceed the original sie
     *
     * ```should_abort
     * char list[] = { 'a', 'b', 'c', 'd', 'e' };
     * Span span1 { list, 5 };
     * span1.slice(2, 5);
     * ```
     */
    Span slice(const size_t offset, size_t count = 0) const {
        assert(offset + count <= m_size);
        if (count == 0)
            count = m_size - offset;
        return { m_data + offset, count };
    }

    /**
     * Return a pointer to the underlying storage array.
     *
     * ```
     * const char list[] = { 'a', 'b', 'c' };
     * Span span { list, 3 };
     * auto *ary = span.data();
     * assert_eq('b', ary[1]);
     * ```
     */
    const T *data() const { return m_data; }

    class iterator {
    public:
        iterator(const Span<T> &span, const size_t index)
            : m_span { span }
            , m_index { index } { }

        iterator operator++() {
            m_index++;
            return *this;
        }

        iterator operator++(int) {
            iterator i = *this;
            m_index++;
            return i;
        }

        const T &operator*() const { return m_span.m_data[m_index]; }
        const T *operator->() const { return m_span.m_data[m_index]; }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_span.data() == i2.m_span.data() && i1.m_index == i2.m_index;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_span.data() != i2.m_span.data() || i1.m_index != i2.m_index;
        }

    private:
        const Span<T> &m_span;
        size_t m_index { 0 };
    };

    /**
     * Returns an iterator over the vector.
     *
     * ```
     * char list[] = { 'a', 'b', 'c' };
     * const Span span { list, 3 };
     * auto it = span.begin();
     * assert_eq('a', *it++);
     * assert_eq('b', *it++);
     * assert_eq('c', *it++);
     * assert(it == span.end());
     * ```
     */
    iterator begin() const {
        return iterator { *this, 0 };
    }

    iterator end() const {
        return iterator { *this, m_size };
    }

private:
    const T *m_data { nullptr };
    size_t m_size { 0 };
};

}
