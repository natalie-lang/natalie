#pragma once

#include "string.hpp"
#include <assert.h>

namespace TM {

class StringView {
public:
    /**
     * Constructs an empty StringView.
     *
     * ```
     * auto view = StringView();
     * assert_eq(0, view.size());
     * assert_str_eq("", view);
     * ```
     */
    StringView() { }

    /**
     * Constructs a basic StringView, pointing at the given String.
     *
     * ```
     * auto str = String("foo");
     * auto view = StringView(&str);
     * assert_eq(3, view.size());
     * ```
     */
    explicit StringView(const String *string)
        : m_string { string }
        , m_length { string->length() } { }

    /**
     * Constructs a StringView with given offset.
     *
     * ```
     * auto str = String("foobar");
     * auto view = StringView(&str, 3);
     * assert_str_eq("bar", view);
     *
     * auto str2 = String("foo");
     * auto view2 = StringView(&str2, 3);
     * assert_str_eq("", view2);
     * ```
     *
     * This constructor aborts if the offset is past the end of the String.
     *
     * ```should_abort
     * auto str = String("foo");
     * auto view = StringView(&str, 4);
     * (void)view;
     * ```
     */
    explicit StringView(const String *string, size_t offset)
        : m_string { string }
        , m_offset { offset }
        , m_length { string->length() - offset } {
        assert(offset <= string->length());
    }

    /**
     * Constructs a StringView with given offset and length.
     *
     * ```
     * auto str = String("foo-bar-baz");
     * auto view = StringView(&str, 4, 3);
     * assert_str_eq("bar", view);
     * ```
     *
     * This constructor aborts if the length is too long.
     *
     * ```should_abort
     * auto str = String("foobar");
     * auto view = StringView(&str, 3, 4);
     * (void)view;
     * ```
     */
    explicit StringView(const String *string, size_t offset, size_t length)
        : m_string { string }
        , m_offset { offset }
        , m_length { length } {
        assert(offset <= string->length());
        assert(length <= string->length() - offset);
    }

    /**
     * Constructs a StringView by copying an existing StringView.
     *
     * ```
     * auto str = String("foo-bar-baz");
     * auto view1 = StringView(&str, 4, 3);
     * auto view2 = StringView(view1);
     * assert_str_eq("bar", view2);
     * ```
     */
    StringView(const StringView &other) = default;

    /**
     * Replaces the StringView by copying from an another StringView.
     *
     * ```
     * auto str1 = String("foo");
     * auto view1 = StringView(&str1);
     * auto str2 = String("bar");
     * auto view2 = StringView(&str2);
     * view1 = view2;
     * assert_str_eq("bar", view1);
     * ```
     */
    StringView &operator=(const StringView &other) = default;

    /**
     * Returns the offset into the String.
     *
     * ```
     * auto str = String("foo-bar-baz");
     * auto view = StringView(&str, 4);
     * assert_eq(4, view.offset());
     * ```
     */
    size_t offset() const { return m_offset; }

    /**
     * Returns the number of bytes in the View.
     *
     * ```
     * auto str = String("🤖"); // 4-byte emoji
     * auto view = StringView(&str);
     * assert_eq(4, view.size());
     *
     * auto str2 = String("foobar");
     * auto view2 = StringView(&str2, 3);
     * assert_eq(3, view2.size());
     * ```
     */
    size_t size() const { return m_length; }

    /**
     * Returns the number of bytes in the View.
     *
     * ```
     * auto str = String("🤖"); // 4-byte emoji
     * auto view = StringView(&str);
     * assert_eq(4, view.length());
     *
     * auto str2 = String("foobar");
     * auto view2 = StringView(&str2, 3);
     * assert_eq(3, view2.length());
     * ```
     */
    size_t length() const { return m_length; }

    /**
     * Returns true if this and the given C string are equivalent.
     *
     * ```
     * auto str = String("abc");
     * auto view = StringView(&str);
     * auto cstr1 = "abc";
     * assert(view == cstr1);
     * auto cstr2 = "xyz";
     * assert_not(view == cstr2);
     * ```
     */
    bool operator==(const char *other) const {
        if (!other && !m_string)
            return true;
        auto other_length = strlen(other);
        if (!m_string && other_length == 0)
            return true;
        if (!m_string)
            return false;
        if (m_length != other_length)
            return false;
        return memcmp(m_string->c_str() + m_offset, other, sizeof(char) * m_length) == 0;
    }

    bool operator!=(const char *other) const {
        return !(*this == other);
    }

    /**
     * Returns true if this and the given StringView are equivalent.
     *
     * ```
     * auto str1 = String("abc");
     * auto view1 = StringView(&str1);
     * auto view1b = StringView(&str1);
     * assert(view1 == view1b);
     *
     * auto str2 = String("xyz");
     * auto view2 = StringView(&str2);
     * assert_not(view1 == view2);
     *
     * assert(StringView() == StringView());
     * ```
     */
    bool operator==(const StringView &other) const {
        if (m_string == other.m_string) // shortcut
            return m_offset == other.m_offset && m_length == other.m_length;
        if (!m_string && other.m_length == 0)
            return true;
        if (!m_string)
            return false;
        if (m_length != other.m_length)
            return false;
        return memcmp(m_string->c_str() + m_offset, other.m_string->c_str(), sizeof(char) * m_length) == 0;
    }

    bool operator!=(const StringView &other) const {
        return !(*this == other);
    }

    /**
     * Returns true if this and the given String are equivalent.
     *
     * ```
     * auto str = String("abc");
     * auto view = StringView(&str);
     * assert(view == str);
     * assert_not(view == String());
     * ```
     */
    bool operator==(const String &other) const {
        return *this == StringView(&other);
    }

    bool operator!=(const String &other) const {
        return !(*this == other);
    }

    /**
     * Returns a new String constructed from this view.
     *
     * ```
     * auto str = String("foo-bar-baz");
     * auto view = StringView(&str, 4, 3);
     * auto str2 = view.to_string();
     * assert_str_eq("bar", str2);
     * ```
     */
    String to_string() const {
        if (!m_string)
            return String();
        return String { m_string->c_str() + m_offset, m_length };
    }

    /**
     * Returns a new String constructed from this view.
     *
     * ```
     * auto str = String("foo-bar-baz");
     * auto view = StringView(&str, 4, 3);
     * auto str2 = view.clone();
     * assert_str_eq("bar", str2);
     * ```
     */
    String clone() const { return to_string(); }

    /**
     * Returns the character at the specified index.
     *
     * ```
     * auto str = String("foo-bar-baz");
     * auto view = StringView(&str, 4, 3);
     * assert_eq('a', view.at(1));
     * ```
     *
     * This method aborts if the given index is beyond the end of the String.
     *
     * ```should_abort
     * auto str = String("foo-bar-baz");
     * auto view = StringView(&str, 4, 3);
     * view.at(10);
     * ```
     */
    char at(size_t index) const {
        assert(m_string);
        return m_string->at(m_offset + index);
    }

    /**
     * Returns the character at the specified index.
     *
     * ```
     * auto str = String("foo-bar-baz");
     * auto view = StringView(&str, 4, 3);
     * assert_eq('a', view[1]);
     * ```
     *
     * WARNING: This method does *not* check that the given
     * index is within the bounds of the string data!
     */
    char operator[](size_t index) const {
        assert(m_string);
        return (*m_string)[m_offset + index];
    }

    /**
     * Returns a pointer to the underlying C string data.
     * This method should be used with care, because it does not
     * contain a null termination character at the expected location.
     * It only points to the offset of the String where this
     * StringView starts.
     *
     * ```
     * auto str = String("foo-bar-baz");
     * auto view = StringView(&str, 4, 3);
     * assert_eq(0, strcmp("bar-baz", view.dangerous_pointer_to_underlying_data()));
     * ```
     */
    const char *dangerous_pointer_to_underlying_data() const {
        assert(m_string);
        return (*m_string).c_str() + m_offset;
    }

    /**
     * Retruns true if the StringView has a length of zero.
     *
     * ```
     * auto str = String("abc");
     * auto view1 = StringView(&str);
     * assert_not(view1.is_empty());
     * auto view2 = StringView(&str, 0, 0);
     * assert(view2.is_empty());
     * ```
     */
    bool is_empty() const { return m_length == 0; }

private:
    const String *m_string { nullptr };
    size_t m_offset { 0 };
    size_t m_length { 0 };
};

}
