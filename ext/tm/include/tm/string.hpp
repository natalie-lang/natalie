#pragma once

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace TM {

class String final {
public:
    static constexpr int STRING_GROW_FACTOR = 2;

    /**
     * Constructs an empty String.
     *
     * ```
     * auto str = String();
     * assert_eq(0, str.size());
     * ```
     */
    String() { }

    /**
     * Constructs a new String by copying the contents
     * from an existing C string.
     *
     * ```
     * auto cstr = "foo";
     * auto str = String(cstr);
     * assert_eq(3, str.size());
     * assert_neq(str.c_str(), cstr);
     * ```
     */
    String(const char *const str) {
        assert(str);
        set_str(str);
    }

    /**
     * Constructs a new String by copying the contents
     * from an existing C string with a given length.
     *
     * The given C string can contain null characters.
     * The full given length gets copied regardless of nulls.
     *
     * ```
     * auto cstr = "foo\0bar";
     * auto str = String(cstr, 7);
     * assert_eq(7, str.size());
     * assert_eq('\0', str[3]);
     * assert_eq('r', str[6]);
     * ```
     */
    String(const char *const str, const size_t length) {
        assert(str);
        set_str(str, length);
    }

    /**
     * Constructs a new String by copying the contents
     * from an existing String.
     *
     * ```
     * auto str1 = String { "foo" };
     * auto str2 = String { str1 };
     * assert_str_eq("foo", str2);
     * ```
     */
    String(const String &other) {
        set_str(other.c_str(), other.size());
    }

    /**
     * Constructs a new String by moving the contents
     * from an existing String.
     *
     * ```
     * auto str1 = String { "foo" };
     * auto str2 = String { std::move(str1) };
     * assert_str_eq("foo", str2);
     * assert_eq(0, str1.size());
     * ```
     */
    String(String &&other) {
        m_str = other.m_str;
        m_length = other.m_length;
        m_capacity = other.m_capacity;

        other.m_str = nullptr;
        other.m_length = 0;
        other.m_capacity = 0;
    }

    /**
     * Constructs a new String by copying the contents
     * from an existing String pointer.
     *
     * ```
     * auto str1 = String { "foo" };
     * auto str2 = String { &str1 };
     * assert_str_eq("foo", str2);
     * ```
     */
    String(const String *const other) {
        assert(other);
        set_str(other->c_str(), other->size());
    }

    /**
     * Constructs a new String with the single given charater.
     *
     * ```
     * auto str = String { 'x' };
     * assert_str_eq("x", str);
     * ```
     */
    String(const char c) {
        const char buf[2] = { c, 0 };
        set_str(buf, 1);
    }

    /**
     * Constructs a new String of the specified length
     * by filling it with the given character.
     *
     * ```
     * auto str = String { 10, 'x' };
     * assert_str_eq("xxxxxxxxxx", str);
     * ```
     */
    String(const size_t length, const char c) {
        if (length == 0) {
            set_str("", 0);
            return;
        }
        char buf[length];
        memset(buf, c, sizeof(char) * length);
        set_str(buf, length);
    }

    /**
     * Constructs a new String by converting the given number.
     *
     * ```
     * auto str = String { (long long)10 };
     * assert_str_eq("10", str);
     * ```
     */
    String(const long long number) {
        const int length = snprintf(NULL, 0, "%lli", number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%lli", number);
        set_str(buf);
    }

    /**
     * Constructs a new String by converting the given number.
     *
     * ```
     * auto str = String { (unsigned long long)10 };
     * assert_str_eq("10", str);
     * ```
     */

    String(const unsigned long long number) {
        const int length = snprintf(NULL, 0, "%llu", number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%llu", number);
        set_str(buf);
    }

    /**
     * Constructs a new String by converting the given number.
     *
     * ```
     * auto str = String { (unsigned long int)10 };
     * auto str2 = String { (long int)10 };
     * assert_str_eq("10", str);
     * assert_str_eq("10", str2);
     * ```
     */
    String(const long int number) {
        const int length = snprintf(NULL, 0, "%li", number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%li", number);
        set_str(buf);
    }

    /**
     * Constructs a new String by converting the given number.
     *
     * ```
     * auto str = String { (int)10 };
     * assert_str_eq("10", str);
     * ```
     */
    String(const int number) {
        const int length = snprintf(NULL, 0, "%d", number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%d", number);
        set_str(buf);
    }

    /**
     * Constructs a new String by converting the given number.
     *
     * ```
     * auto str = String { (unsigned long)10 };
     * assert_str_eq("10", str);
     * ```
     */
    String(const unsigned long number) {
        const int length = snprintf(NULL, 0, "%lu", number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%lu", number);
        set_str(buf);
    }

    /**
     * Constructs a new String by converting the given number.
     *
     * ```
     * auto str = String { (unsigned int)10 };
     * assert_str_eq("10", str);
     * ```
     */
    String(const unsigned int number) {
        const int length = snprintf(NULL, 0, "%u", number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%u", number);
        set_str(buf);
    }

    /**
     * Constructs a new String by converting the given
     * double-precision float.
     *
     * ```
     * auto str = String { 4.1 };
     * assert_str_eq("4.1000", str);
     * ```
     *
     * You can optionally specify the decimal precision.
     *
     * ```
     * auto str = String { 4.1, 1 };
     * assert_str_eq("4.1", str);
     * ```
     */
    String(const double number, const int precision = 4) {
        const int length = snprintf(NULL, 0, "%.*f", precision, number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%.*f", precision, number);
        set_str(buf);
    }

    enum class HexFormat {
        UppercaseAndPrefixed,
        Uppercase,
        LowercaseAndPrefixed,
        Lowercase,
    };

    /**
     * Creates a new String by converting the given
     * number to hexadecimal format. By default, the
     * result will be uppercase and prefixed with "0X".
     *
     * ```
     * auto str = String::hex(254);
     * assert_str_eq("0XFE", str);
     * ```
     *
     * You can optionally specify the format, one of:
     * - HexFormat::UppercaseAndPrefixed
     * - HexFormat::Uppercase
     * - HexFormat::LowercaseAndPrefixed
     * - HexFormat::Lowercase
     *
     * ```
     * auto str = String::hex(254, String::HexFormat::Lowercase);
     * assert_str_eq("fe", str);
     * ```
     */
    static String hex(const long long number, HexFormat format = HexFormat::UppercaseAndPrefixed) {
        const bool uppercase = format == HexFormat::UppercaseAndPrefixed || format == HexFormat::Uppercase;
        const bool prefixed = format == HexFormat::UppercaseAndPrefixed || format == HexFormat::LowercaseAndPrefixed;
        const char *format_str = uppercase ? "%llX" : "%llx";
        const int length = snprintf(NULL, 0, format_str, number);
        char buf[length + 1];
        snprintf(buf, length + 1, format_str, number);
        auto str = String(buf);
        if (prefixed)
            str.prepend(uppercase ? "0X" : "0x");
        return str;
    }

    ~String() {
        delete[] m_str;
    }

    /**
     * Replaces the String data by copying from an another String.
     *
     * ```
     * auto str1 = String { "foo" };
     * auto str2 = String { str1 };
     * assert_str_eq("foo", str2);
     * ```
     */
    String &operator=(const String &other) {
        if (m_str == other.m_str)
            m_length = other.m_length;
        else
            set_str(other.c_str(), other.size());
        return *this;
    }

    /**
     * Replaces the String data by moving from an another String.
     *
     * ```
     * auto str1 = String { "foo" };
     * auto str2 = String { std::move(str1) };
     * assert_str_eq("foo", str2);
     * assert_eq(0, str1.size());
     * ```
     */
    String &operator=(String &&other) {
        if (m_str == other.m_str)
            m_length = other.m_length;
        else {
            delete[] m_str;

            m_str = other.m_str;
            m_length = other.m_length;
            m_capacity = other.m_capacity;

            other.m_str = nullptr;
            other.m_length = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    /**
     * Appends two Strings together and returns the result.
     *
     * ```
     * auto str1 = String { "foo" };
     * auto str2 = String { "bar" };
     * assert_str_eq("foobar", str1 + str2);
     *
     * assert_str_eq("12", String("1") + "2");
     * ```
     */
    String operator+(const String &other) const {
        auto new_string = String(*this);
        new_string.append(other);
        return new_string;
    }

    /**
     * Replaces the String data by copying from an a C string.
     *
     * ```
     * auto cstr = "foo";
     * auto str = String { cstr };
     * assert_str_eq("foo", str);
     * ```
     */
    String &operator=(const char *const other) {
        if (other[0] == '\0') {
            truncate(0);
            return *this;
        }
        set_str(other);
        return *this;
    }

    /**
     * Returns the character at the specified index.
     *
     * ```
     * auto str = String { "abc" };
     * assert_eq('b', str.at(1));
     * ```
     *
     * This method aborts if the given index is beyond the end of the String.
     *
     * ```should_abort
     * auto str = String { "abc" };
     * str.at(10);
     * ```
     */
    char at(const size_t index) const {
        assert(index < m_length);
        return m_str[index];
    }

    /**
     * Returns the character at the specified index.
     *
     * ```
     * auto str = String { "abc" };
     * assert_eq('b', str[1]);
     * ```
     *
     * WARNING: This method does *not* check that the given
     * index is within the bounds of the string data!
     */
    char operator[](const size_t index) const {
        return m_str[index];
    }

    /**
     * Returns a reference to the character at the specified index.
     *
     * ```
     * auto str = String { "abc" };
     * assert_eq('b', str[1]);
     * ```
     *
     * This allows you to set the character at the given index.
     *
     * ```
     * auto str = String { "abc" };
     * str[1] = 'r';
     * assert_eq('r', str[1]);
     * ```
     *
     * WARNING: This method does *not* check that the given
     * index is within the bounds of the string data!
     */
    char &operator[](const size_t index) {
        return m_str[index];
    }

    /**
     * Returns the last character in the string.
     *
     * ```
     * auto str = String { "abc" };
     * assert_eq('c', str.last_char());
     * ```
     *
     * This method aborts if the String is zero-length.
     *
     * ```should_abort
     * auto str = String { "" };
     * str.last_char();
     * ```
     */
    char last_char() const {
        assert(m_length > 0);
        return m_str[m_length - 1];
    }

    /**
     * Removes the last character from the string and returns it.
     *
     * ```
     * auto str = String { "abc" };
     * assert_eq('c', str.pop_char());
     * assert_eq('b', str.pop_char());
     * assert_eq(1, str.size());
     * ```
     *
     * This method aborts if the String is zero-length.
     *
     * ```should_abort
     * auto str = String { "" };
     * str.pop_char();
     * ```
     */
    char pop_char() {
        assert(m_length > 0);
        return m_str[--m_length];
    }

    /**
     * Returns a new copy of the String.
     *
     * ```
     * auto str1 = String { "abc" };
     * auto str2 = str1.clone();
     * assert_str_eq("abc", str2);
     * assert_neq(str1.c_str(), str2.c_str()); // not the same pointer
     * ```
     */
    String clone() const { return String { *this }; }

    /**
     * Returns a String created from a substring of this one.
     * Pass a start index and the desired length.
     *
     * ```
     * auto str1 = String { "abc" };
     * auto str2 = str1.substring(1, 2);
     * assert_str_eq("bc", str2);
     * ```
     *
     * This method aborts if the given start index is past the end.
     *
     * ```should_abort
     * auto str = String { "abc" };
     * str.substring(3, 1);
     * ```
     *
     * ...and if the resulting end index (start + length) is past the end.
     *
     * ```should_abort
     * auto str = String { "abc" };
     * str.substring(1, 3);
     * ```
     */
    String substring(const size_t start, const size_t length) const {
        assert(start < m_length);
        assert(start + length <= m_length);
        return String(c_str() + start, length);
    }

    /**
     * Returns a String created from a substring of this one.
     * Pass a start index. (All characters to the end of the string
     * will be included.)
     *
     * ```
     * auto str1 = String { "abc" };
     * auto str2 = str1.substring(1);
     * assert_str_eq("bc", str2);
     * ```
     *
     * This method aborts if the given start index is past the end.
     *
     * ```should_abort
     * auto str = String { "abc" };
     * str.substring(3);
     * ```
     */
    String substring(const size_t start) const {
        return substring(start, m_length - start);
    }

    /**
     * Returns a C string pointer to the internal data.
     *
     * ```
     * auto str = String { "abc" };
     * auto cstr = str.c_str();
     * assert_eq(0, strcmp(cstr, "abc"));
     * ```
     */
    const char *c_str() const { return m_str ? m_str : ""; }

    /**
     * Returns the number of bytes in the String.
     *
     * ```
     * auto str = String { "🤖" }; // 4-byte emoji
     * assert_eq(4, str.length());
     * ```
     */
    size_t length() const { return m_length; }

    /**
     * Returns the number of bytes in the String.
     *
     * ```
     * auto str = String { "🤖" }; // 4-byte emoji
     * assert_eq(4, str.size());
     * ```
     */
    size_t size() const { return m_length; }

    /**
     * Returns the number of bytes available in internal storage.
     *
     * ```
     * auto str = String { "abc" };
     * str.append_char('d');
     * assert_eq(6, str.capacity()); // the capacity is doubled
     * ```
     */
    size_t capacity() const { return m_capacity; }

    /**
     * Overwrites the String with the given C string.
     *
     * ```
     * auto str = String { "abc" };
     * str.set_str("xyz");
     * assert_str_eq("xyz", str);
     * ```
     */
    void set_str(const char *const str) {
        set_str(str, strlen(str));
    }

    /**
     * Overwrites the String with the given C string with
     * specified length.
     *
     * The given C string can contain null characters.
     * The full given length gets copied regardless of nulls.
     *
     * ```
     * auto str = String { "abc" };
     * str.set_str("def\0ghi", 7);
     * assert_eq(7, str.size());
     * assert_eq('d', str[0]);
     * assert_eq('\0', str[3]);
     * assert_eq('i', str[6]);
     * ```
     */
    void set_str(const char *const str, const size_t length) {
        assert(str);
        if (m_capacity > 0 && length <= m_capacity) {
            memcpy(m_str, str, sizeof(char) * length);
            m_str[length] = 0;
            m_length = length;
            return;
        }
        delete[] m_str;
        m_str = new char[length + 1];
        memcpy(m_str, str, sizeof(char) * length);
        m_str[length] = 0;
        m_length = length;
        m_capacity = length;
    }

    /**
     * Inserts the given character character at the front
     * of the string, shifting all the other strings down by one.
     *
     * ```
     * auto str = String { "23" };
     * str.prepend_char('1');
     * assert_str_eq("123", str);
     * ```
     */
    void prepend_char(const char c) {
        const size_t total_length = m_length + 1;
        grow_at_least(total_length);
        memmove(m_str + 1, m_str, m_length + 1); // 1 extra for null terminator
        m_str[0] = c;
        m_length = total_length;
    }

    /**
     * Converts the given number and prepends the resulting string.
     *
     * ```
     * auto str = String { "abc" };
     * str.prepend(123);
     * assert_str_eq("123abc", str);
     * ```
     */
    void prepend(long long i) {
        const int length = snprintf(NULL, 0, "%lli", i);
        char buf[length + 1];
        snprintf(buf, length + 1, "%lli", i);
        prepend(buf);
    }

    /**
     * Prepends the given C string.
     *
     * ```
     * auto str = String { "def" };
     * str.prepend("abc");
     * assert_str_eq("abcdef", str);
     * ```
     */
    void prepend(const char *const str) {
        if (!str) return;
        const size_t new_length = strlen(str);
        if (new_length == 0) return;
        grow_at_least(new_length + m_length);
        memmove(m_str + new_length, m_str, sizeof(char) * (m_length + 1));
        memcpy(m_str, str, sizeof(char) * new_length);
        m_length += new_length;
    }

    /**
     * Prepends the given String.
     *
     * ```
     * auto str1 = String { "def" };
     * auto str2 = String { "abc" };
     * str1.prepend(str2);
     * assert_str_eq("abcdef", str1);
     * ```
     */
    void prepend(const String &str) {
        const size_t new_length = str.size();
        if (new_length == 0) return;
        grow_at_least(new_length + m_length);
        memmove(m_str + new_length, m_str, sizeof(char) * (m_length + 1));
        memcpy(m_str, str.c_str(), sizeof(char) * new_length);
        m_length += new_length;
    }

    /**
     * Inserts at the specified index the character given.
     *
     * ```
     * auto str = String { "xyz" };
     * str.insert(0, '-');
     * str.insert(2, '-');
     * str.insert(4, '-');
     * assert_str_eq("-x-y-z", str);
     * ```
     *
     * This method aborts if the index is past the end.
     *
     * ```should_abort
     * auto str = String { "xxx" };
     * str.insert(3, '-');
     * ```
     */
    void insert(const size_t index, const char c) {
        assert(index < m_length);
        grow_at_least(m_length + 1);
        const size_t nbytes = m_length - index + 1; // 1 extra for null terminator
        memmove(m_str + index + 1, m_str + index, nbytes);
        m_str[index] = c;
        m_length++;
    }

    /**
     * Replaces the specified index+size bytes with the String given.
     *
     * ```
     * auto str = String { "foo-bar-baz" };
     * str.replace_bytes(4, 3, "buz");
     * assert_str_eq("foo-buz-baz", str);
     * str.replace_bytes(4, 3, "b");
     * assert_str_eq("foo-b-baz", str);
     * str.replace_bytes(4, 1, "bar");
     * assert_str_eq("foo-bar-baz", str);
     * str.replace_bytes(10, 1, "a");
     * assert_str_eq("foo-bar-baa", str);
     * ```
     */
    void replace_bytes(const size_t index, const size_t length, const String &replacement) {
        assert(index < m_length);
        assert(index + length <= m_length);
        const ssize_t diff = replacement.size() - length;
        if (diff > 0)
            grow_at_least(m_length + diff);
        if (diff != 0) {
            const auto src = index + length;
            const auto dest = src + diff;
            memmove(m_str + dest, m_str + src, m_length - index - length);
        }
        for (size_t i = 0; i < replacement.size(); i++)
            m_str[index + i] = replacement[i];
        m_length += diff;
        m_str[m_length] = 0;
    }

    /**
     * Adds the given character at the end of the string.
     *
     * ```
     * auto str = String { "ab" };
     * str.append_char('c');
     * assert_str_eq("abc", str);
     * ```
     */
    void append_char(const char c) {
        const size_t total_length = m_length + 1;
        grow_at_least(total_length);
        m_str[total_length - 1] = c;
        m_str[total_length] = 0;
        m_length = total_length;
    }

    /**
     * Adds the given signed character at the end of the string.
     *
     * ```
     * auto str = String { "ab" };
     * str.append((signed char)'c');
     * assert_str_eq("abc", str);
     * ```
     */
    void append(const signed char c) {
        const size_t total_length = m_length + 1;
        grow_at_least(total_length);
        m_str[total_length - 1] = c;
        m_str[total_length] = 0;
        m_length = total_length;
    }

    /**
     * Adds the given unsigned character at the end of the string.
     *
     * ```
     * auto str = String { "ab" };
     * str.append((unsigned char)'c');
     * assert_str_eq("abc", str);
     * ```
     */
    void append(const unsigned char c) {
        append(static_cast<signed char>(c));
    }

    /**
     * Adds the given character at the end of the string.
     *
     * ```
     * auto str = String { "ab" };
     * str.append((char)'c');
     * assert_str_eq("abc", str);
     * ```
     */
    void append(const char c) {
        append(static_cast<signed char>(c));
    }

    /**
     * Converts the given number and append the resulting string.
     *
     * ```
     * auto str = String { "a" };
     * str.append((size_t)123);
     * assert_str_eq("a123", str);
     * ```
     */
    void append(const size_t i) {
        const int length = snprintf(NULL, 0, "%zu", i);
        char buf[length + 1];
        snprintf(buf, length + 1, "%zu", i);
        append(buf);
    }

    /**
     * Converts the given number and append the resulting string.
     *
     * ```
     * auto str = String { "a" };
     * str.append((long long)123);
     * assert_str_eq("a123", str);
     * ```
     */
    void append(const long long i) {
        const int length = snprintf(NULL, 0, "%lli", i);
        char buf[length + 1];
        snprintf(buf, length + 1, "%lli", i);
        append(buf);
    }

    /**
     * Converts the given number and append the resulting string.
     *
     * ```
     * auto str = String { "a" };
     * str.append((int)123);
     * assert_str_eq("a123", str);
     * ```
     */
    void append(const int i) {
        const int length = snprintf(NULL, 0, "%i", i);
        char buf[length + 1];
        snprintf(buf, length + 1, "%i", i);
        append(buf);
    }

    /**
     * Appends the given C string.
     *
     * ```
     * auto str = String { "a" };
     * str.append("bc");
     * assert_str_eq("abc", str);
     * ```
     */
    void append(const char *const str) {
        if (!str) return;
        const size_t length = strlen(str);
        if (length == 0) return;
        append(str, length);
    }

    /**
     * Appends the given C string with specified length.
     *
     * The given C string can contain null characters.
     * The full given length gets append regardless of nulls.
     *
     * ```
     * auto str = String { "x" };
     * str.append("abc\0def", 7);
     * assert_eq(8, str.size());
     * assert_eq('x', str[0]);
     * assert_eq('\0', str[4]);
     * assert_eq('f', str[7]);
     * ```
     */
    void append(const char *const str, const size_t length) {
        if (!str) return;
        if (length == 0) return;
        const size_t total_length = m_length + length;
        grow_at_least(total_length);
        memcpy(m_str + m_length, str, length);
        m_str[total_length] = 0;
        m_length = total_length;
    }

    /**
     * Appends the given arguments, formatting using the specified format.
     *
     * This is roughly equivalent to constructing a C string using sprintf()
     * and then appending the result to this String.
     *
     * ```
     * auto str = String { "x" };
     * str.append_sprintf("y%c%d", 'z', 1);
     * assert_str_eq("xyz1", str);
     * ```
     */
    void append_sprintf(const char *const format, ...) {
        va_list args;
        va_start(args, format);
        append_vsprintf(format, args);
        va_end(args);
    }

    /**
     * Appends the given va_list args, formatting using the specified format.
     */
    void append_vsprintf(const char *const format, va_list args) {
        va_list args_copy;
        va_copy(args_copy, args);
        const int fmt_length = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);
        char buf[fmt_length + 1];
        vsnprintf(buf, fmt_length + 1, format, args);
        append(buf);
    }

    /**
     * Appends the given String.
     *
     * ```
     * auto str1 = String { "x" };
     * auto str2 = String { "yz" };
     * str1.append(str2);
     * assert_str_eq("xyz", str1);
     * ```
     */
    void append(const String &str) {
        if (str.size() == 0) return;
        const size_t total_length = m_length + str.size();
        grow_at_least(total_length);
        memcpy(m_str + m_length, str.c_str(), sizeof(char) * str.size());
        m_length = total_length;
        m_str[m_length] = 0;
    }

    /**
     * Repeatedly adds the given number of the given character.
     *
     * ```
     * auto str = String { "x" };
     * str.append(2, 'y');
     * assert_str_eq("xyy", str);
     * ```
     */
    void append(const size_t n, const char c) {
        const size_t total_length = m_length + n;
        grow_at_least(total_length);
        memset(m_str + m_length, c, sizeof(char) * n);
        m_length = total_length;
        m_str[m_length] = 0;
    }

    /**
     * Returns true if this and the given String are equivalent.
     *
     * ```
     * auto str1 = String { "abc" };
     * auto str2 = String { "abc" };
     * assert(str1 == str2);
     * auto str3 = String { "xyz" };
     * assert_not(str1 == str3);
     * ```
     */
    bool operator==(const String &other) const {
        if (size() != other.size())
            return false;
        return memcmp(c_str(), other.c_str(), sizeof(char) * m_length) == 0;
    }

    bool operator!=(const String &other) const {
        return !operator==(other);
    }

    /**
     * Returns true if this and the given C string are equivalent.
     *
     * ```
     * auto str = String { "abc" };
     * auto cstr1 = "abc";
     * assert(str == cstr1);
     * auto cstr2 = "xyz";
     * assert_not(str == cstr2);
     * ```
     */
    bool operator==(const char *const other) const {
        assert(other);
        if (size() != strlen(other))
            return false;
        return memcmp(c_str(), other, sizeof(char) * m_length) == 0;
    }

    bool operator!=(const char *other) const {
        return !(*this == other);
    }

    /**
     * Returns true if this is alphanumerically greater than
     * the given String.
     *
     * ```
     * auto str1 = String { "def" };
     * auto str2 = String { "abc" };
     * assert(str1 > str2);
     * assert_not(str2 > str1);
     * ```
     *
     * This works also with strings containing a null character
     * in the middle.
     *
     * ```
     * auto str1 = String { "abc\0def", 7 };
     * auto str2 = String { "abc\0abc", 7 };
     * auto str3 = String { "abc\0abc\0def", 11 };
     * assert(str1 > str2);
     * assert(str3 > str2);
     * ```
     */
    bool operator>(const String &other) const {
        return cmp(other) > 0;
    }

    /**
     * Returns true if this is alphanumerically greater than
     * or equal to the given String.
     *
     * ```
     * auto str1 = String { "def" };
     * auto str2 = String { "abc" };
     * auto str3 = String { "abc" };
     * assert(str1 >= str2);
     * assert(str2 >= str3);
     * assert_not(str2 >= str1);
     * ```
     *
     * This works also with strings containing a null character
     * in the middle.
     *
     * ```
     * auto str1 = String { "abc\0def", 7 };
     * auto str2 = String { "abc\0abc", 7 };
     * auto str3 = String { "abc\0abc", 7 };
     * auto str4 = String { "abc\0abc\0def", 11 };
     * assert(str1 >= str2);
     * assert(str2 >= str3);
     * assert(str4 >= str2);
     * assert_not(str2 >= str1);
     * ```
     */
    bool operator>=(const String &other) const {
        return cmp(other) >= 0;
    }

    /**
     * Returns true if this is alphanumerically less than
     * the given String.
     *
     * ```
     * auto str1 = String { "abc" };
     * auto str2 = String { "def" };
     * assert(str1 < str2);
     * assert_not(str2 < str1);
     * ```
     *
     * This works also with strings containing a null character
     * in the middle.
     *
     * ```
     * auto str1 = String { "abc\0abc", 7 };
     * auto str2 = String { "abc\0def", 7 };
     * auto str3 = String { "abc\0def\0def", 11 };
     * assert(str1 < str2);
     * assert(str2 < str3);
     * ```
     */
    bool operator<(const String &other) const {
        return cmp(other) < 0;
    }

    /**
     * Returns true if this is alphanumerically less than
     * or equal to the given String.
     *
     * ```
     * auto str1 = String { "abc" };
     * auto str2 = String { "def" };
     * auto str3 = String { "def" };
     * assert(str1 <= str2);
     * assert(str2 <= str3);
     * assert_not(str2 <= str1);
     * ```
     *
     * This works also with strings containing a null character
     * in the middle.
     *
     * ```
     * auto str1 = String { "abc\0abc", 7 };
     * auto str2 = String { "abc\0def", 7 };
     * auto str3 = String { "abc\0def", 7 };
     * auto str4 = String { "abc\0def\0def", 11 };
     * assert(str1 <= str2);
     * assert(str2 <= str3);
     * assert(str2 <= str4);
     * assert_not(str2 <= str1);
     * ```
     */
    bool operator<=(const String &other) const {
        return cmp(other) <= 0;
    }

    /**
     * Returns -1, 0, or 1 by comparing this String to the given String.
     * -1 is returned if this String is alphanumerically less than the other one.
     * 0 is returned if they are equivalent.
     * 1 is returned if this String is alphanumerically greater than the other one.
     *
     * ```
     * auto str1 = String { "def" };
     * auto str2 = String { "abc" };
     * assert_eq(1, str1.cmp(str2));
     * assert_eq(-1, str2.cmp(str1));
     * auto str3 = String { "abc" };
     * assert_eq(0, str2.cmp(str3));
     * auto str4 = String { "abcabc" };
     * assert_eq(-1, str2.cmp(str4));
     * ```
     */
    int cmp(const String &other) const {
        if (m_length == 0) {
            if (other.m_length == 0)
                return 0;
            return -1;
        }
        for (size_t i = 0; i < std::min(m_length, other.m_length); ++i) {
            const auto c1 = (unsigned char)(*this)[i], c2 = (unsigned char)other[i];
            if (c1 < c2)
                return -1;
            else if (c1 > c2)
                return 1;
        }
        if (m_length == other.m_length)
            return 0;
        else if (m_length < other.m_length)
            return -1;
        else
            return 1;
    }

    /**
     * Returns -1, 0, or 1 by comparing this String to the given String,
     * ignoring difference in case.
     * -1 is returned if this String is alphanumerically less than the other one.
     * 0 is returned if they are equivalent.
     * 1 is returned if this String is alphanumerically greater than the other one.
     *
     * ```
     * auto str1 = String { "def" };
     * auto str2 = String { "DEF" };
     * auto str3 = String { "efg" };
     * assert_eq(0, str1.casecmp(str2));
     * assert_eq(-1, str2.casecmp(str3));
     * auto str4 = String { "defdef" };
     * assert_eq(-1, str2.casecmp(str4));
     * ```
     */
    int casecmp(const String &other) const {
        if (m_length == 0) {
            if (other.m_length == 0)
                return 0;
            return -1;
        }
        auto lower = [&](char c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; };
        for (size_t i = 0; i < std::min(m_length, other.m_length); ++i) {
            const auto c1 = lower((unsigned char)(*this)[i]), c2 = lower((unsigned char)other[i]);
            if (c1 < c2)
                return -1;
            else if (c1 > c2)
                return 1;
        }
        if (m_length == other.m_length)
            return 0;
        else if (m_length < other.m_length)
            return -1;
        else
            return 1;
    }

    /**
     * Finds the given String inside this one and return its starting index.
     * If not found, return -1.
     *
     * ```
     * auto str1 = String { "hello world" };
     * auto str2 = String { "lo" };
     * assert_eq(3, str1.find(str2));
     * auto str3 = String { "xx" };
     * assert_eq(-1, str1.find(str3));
     * ```
     */
    ssize_t find(const String &needle) const {
        if (m_length < needle.size() || needle.is_empty())
            return -1;
        assert(m_str);
        const size_t max_index = m_length - needle.size();
        const size_t byte_count = sizeof(char) * needle.size();
        for (size_t index = 0; index <= max_index; ++index) {
            if (memcmp(m_str + index, needle.c_str(), byte_count) == 0)
                return index;
        }
        return -1;
    }

    /**
     * Finds the given character inside this String and return its starting index.
     * If not found, return -1.
     *
     * ```
     * auto str = String { "hello world" };
     * assert_eq(6, str.find('w'));
     * assert_eq(-1, str.find('x'));
     * ```
     */
    ssize_t find(const char c) const {
        for (size_t i = 0; i < m_length; ++i) {
            if (c == m_str[i]) return i;
        }
        return -1;
    }

    /**
     * Truncates this String to the specified length.
     *
     * ```
     * auto str = String { "abcdef" };
     * str.truncate(3);
     * assert_str_eq("abc", str);
     * ```
     *
     * This method aborts if the given length is longer
     * than the String currently is.
     *
     * ```should_abort
     * auto str = String { "abc" };
     * str.truncate(4);
     * ```
     */
    void truncate(const size_t length) {
        assert(length <= m_length);
        if (length == 0) {
            delete[] m_str;
            m_str = nullptr;
            m_length = 0;
            m_capacity = 0;
        } else {
            m_str[length] = 0;
            m_length = length;
        }
    }

    /**
     * Truncates this String to a length of zero.
     *
     * ```
     * auto str = String { "abcdef" };
     * str.clear();
     * assert_eq(0, str.size());
     * ```
     */
    void clear() { truncate(0); }

    /**
     * Removes one character from the end of the String,
     * if this String is not already empty.
     *
     * ```
     * auto str = String { "ab" };
     * str.chomp();
     * assert_str_eq("a", str);
     * str.chomp();
     * assert_str_eq("", str);
     * str.chomp();
     * assert_str_eq("", str);
     * ```
     */
    void chomp() {
        if (m_length == 0) return;
        truncate(m_length - 1);
    }

    /**
     * Removes any trailing whitespace, including tabs and newlines,
     * from the end of the String.
     *
     * ```
     * auto str = String { "a \t\n " };
     * str.strip_trailing_whitespace();
     * assert_str_eq("a", str);
     * ```
     */
    void strip_trailing_whitespace() {
        while (m_length > 0) {
            switch (m_str[m_length - 1]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                chomp();
                break;
            default:
                return;
            }
        }
    }

    /**
     * Removes any trailing spaces from the end of the String.
     *
     * ```
     * auto str = String { "a\n  " };
     * str.strip_trailing_spaces();
     * assert_str_eq("a\n", str);
     * ```
     */
    void strip_trailing_spaces() {
        while (m_length > 0) {
            if (m_str[m_length - 1] == ' ')
                chomp();
            else
                return;
        }
    }

    /**
     * Removes all occurrences of the given character
     * from the String.
     *
     * ```
     * auto str = String { "abcabac" };
     * str.remove('a');
     * assert_str_eq("bcbc", str);
     * ```
     */
    void remove(const char character) {
        assert(m_str);
        for (size_t i = 0; i < m_length; ++i) {
            if (m_str[i] == character) {
                for (size_t j = i; j < m_length; ++j)
                    m_str[j] = m_str[j + 1];

                --m_length;
                --i;
            }
        }
        m_str[m_length] = '\0';
    }

    /**
     * Retruns true if the String has a length of zero.
     *
     * ```
     * auto str1 = String { "abc" };
     * auto str2 = String { "" };
     * assert_not(str1.is_empty());
     * assert(str2.is_empty());
     * ```
     */
    bool is_empty() const { return m_length == 0; }

    /**
     * Returns true if the String contains only
     * characters 0-9.
     *
     * ```
     * auto str1 = String { "1234" };
     * auto str2 = String { "1234abc" };
     * assert(str1.contains_only_digits());
     * assert_not(str2.contains_only_digits());
     * ```
     */
    bool contains_only_digits() const {
        for (size_t i = 0; i < m_length; ++i)
            if (at(i) < '0' || at(i) > '9')
                return false;
        return true;
    }

    /**
     * Returns a new String that is the result of incrementing
     * the last character of this String. If the the last character
     * is z/Z/9, then the next-to-last character is incremented
     * (or a new one is prepended) and the last character is reset.
     *
     * ```
     * assert_str_eq("b",    String("a").successive());
     * assert_str_eq("az",   String("ay").successive());
     * assert_str_eq("ba",   String("az").successive());
     * assert_str_eq("aaa",  String("zz").successive());
     * assert_str_eq("AAA",  String("ZZ").successive());
     * assert_str_eq("1",    String("0").successive());
     * assert_str_eq("100",  String("99").successive());
     * assert_str_eq("d000", String("c999").successive());
     * ```
     */
    String successive() const {
        auto result = String { *this };
        if (m_length == 0) return result;
        const size_t index = size() - 1;
        const char last_char = m_str[index];
        if (last_char == 'z') {
            result.increment_successive_char('a', 'z', 'a');
        } else if (last_char == 'Z') {
            result.increment_successive_char('A', 'Z', 'A');
        } else if (last_char == '9') {
            result.increment_successive_char('0', '9', '1');
        } else {
            result.m_str[index]++;
        }
        return result;
    }

    /**
     * Returns a new String by appending the given arguments according
     * to the given format. This is a safer version of String::sprintf
     * that does not rely on format specifiers matching the argument type.
     *
     * ```
     * auto cstr = "hello";
     * char c = 'w';
     * int num = 999;
     * auto str = String::format("{} {}orld {}", cstr, c, num);
     * assert_str_eq("hello world 999", str);
     * ```
     */
    template <typename... Args>
    static String format(const char *const fmt, Args... args) {
        String out {};
        format(out, fmt, args...);
        return out;
    }

    static void format(String &out, const char *fmt) {
        for (const char *c = fmt; *c != 0; c++) {
            out.append_char(*c);
        }
    }

    template <typename T, typename... Args>
    static void format(String &out, const char *fmt, T first, Args... rest) {
        for (const char *c = fmt; *c != 0; c++) {
            if (*c == '{' && *(c + 1) == '}') {
                out.append(first);
                format(out, c + 2, rest...);
                return;
            } else {
                out.append_char(*c);
            }
        }
    }

    /**
     * Returns a new String where every character is converted to uppercase.
     *
     * ```
     * auto str = String("hElLo");
     * assert_str_eq("HELLO", str.uppercase());
     * ```
     */
    String uppercase() const {
        auto new_str = String(this);
        for (size_t i = 0; i < new_str.m_length; ++i) {
            new_str.m_str[i] = toupper(new_str.m_str[i]);
        }
        return new_str;
    }

    /**
     * Returns a new String where every character is converted to lowercase.
     *
     * ```
     * auto str = String("hElLo");
     * assert_str_eq("hello", str.lowercase());
     * ```
     */
    String lowercase() const {
        auto new_str = String(this);
        for (size_t i = 0; i < new_str.m_length; ++i) {
            new_str.m_str[i] = tolower(new_str.m_str[i]);
        }
        return new_str;
    }

    /**
     * Returns true if this String ends with the given String.
     *
     * ```
     * auto str = String("hello world");
     * assert(str.ends_with("world"));
     * assert_not(str.ends_with("xxx"));
     * ```
     */
    bool ends_with(const String &needle) {
        if (m_length < needle.m_length)
            return false;
        return memcmp(m_str + m_length - needle.m_length, needle.m_str, needle.m_length) == 0;
    }

    /**
     * Returns hash value of this String.
     * This uses the 'djb2' hash algorithm by Dan Bernstein.
     *
     * ```
     * auto str = String("hello");
     * assert_eq(210714636441, str.djb2_hash());
     * ```
     */
    size_t djb2_hash() const {
        size_t hash = 5381;
        int c;
        for (size_t i = 0; i < m_length; ++i) {
            c = (*this)[i];
            hash = ((hash << 5) + hash) + c;
        }
        return hash;
    }

    /**
     * Prints the full string with printf(), character by character.
     * This method will print the full String, even if null characters
     * are encountered.
     *
     * ```
     * auto str = String("foo\0bar");
     * str.print();
     * ```
     */
    void print() const {
        for (size_t i = 0; i < m_length; ++i) {
            printf("%c", (*this)[i]);
        }
        printf("\n");
    }

    /**
     * Returns true if the string contains UTF-8-encoded
     * characters that seem to be valid, multibyte or not.
     *
     * An ASCII string "foo" would return true, because it's
     * also a valid UTF-8-encoded string. It can be
     * represented with UTF-8.
     *
     * NOTE: This is not fool-proof. There's a lot of
     * checks we aren't doing.
     *
     * ```
     * assert_eq(true, String("abc").contains_seemingly_valid_utf8_encoded_characters());
     * assert_eq(true, String("🐄").contains_seemingly_valid_utf8_encoded_characters());
     * assert_eq(false, String("\xC3").contains_seemingly_valid_utf8_encoded_characters());
     * ```
     */
    bool contains_seemingly_valid_utf8_encoded_characters() const {
        int index = 0;
        char buf[5];
        do {
            index = next_utf8_char(index, buf);
        } while (index > 0);
        return index == 0;
    }

    /**
     * Returns true if the string contains multibyte
     * UTF-8-encoded characters that seem to be valid.
     *
     * NOTE: This is not fool-proof. There's a lot of
     * checks we aren't doing.
     *
     * ```
     * assert_eq(false, String("abc").contains_utf8_encoded_multibyte_characters());
     * assert_eq(true, String("🐄").contains_utf8_encoded_multibyte_characters());
     * assert_eq(false, String("\xC3").contains_utf8_encoded_multibyte_characters());
     * ```
     */
    bool contains_utf8_encoded_multibyte_characters() const {
        int index_was = 0;
        int index = 0;
        char buf[5];
        bool multibyte = false;
        do {
            index = next_utf8_char(index, buf);
            if (index > index_was + 1)
                multibyte = true;
            index_was = index;
        } while (index > 0);
        return index == 0 && multibyte;
    }

    /**
     * Takes an integer for the starting index and a
     * buffer char*. Consumes the proper number of
     * bytes, appending to the buffer C string to build
     * a single UTF-8-encoded character.
     *
     * Be sure to pass a pointer to a buffer that is
     * at least 5 characters in size (one is used for
     * the null terminator).
     *
     * Returns the new index, a positive integer,
     * on success.
     *
     * Returns zero (0) if the end of the string has
     * been reached.
     *
     * Returns a negative integer if the character
     * is invalid.
     *
     * (Note: this method only does rudimentary checking
     * for a valid character, i.e. does it have enough
     * bytes to satisfy the encoding.)
     *
     * ```
     * auto str = String("ab☺🐄");
     * int index = 0;
     *
     * char buf[5];
     * index = str.next_utf8_char(index, buf);
     * assert_eq(1, index);
     * assert_str_eq("a", String(buf));
     *
     * buf[0] = '\0';
     * index = str.next_utf8_char(index, buf);
     * assert_eq(2, index);
     * assert_str_eq("b", String(buf));
     *
     * buf[0] = '\0';
     * index = str.next_utf8_char(index, buf);
     * assert_eq(5, index);
     * assert_str_eq("☺", String(buf));
     *
     * buf[0] = '\0';
     * index = str.next_utf8_char(index, buf);
     * assert_eq(9, index);
     * assert_str_eq("🐄", String(buf));
     *
     * buf[0] = '\0';
     * index = str.next_utf8_char(index, buf);
     * assert_eq(0, index);
     * assert_str_eq("", String(buf));
     * ```
     */
    int next_utf8_char(int index, char *buffer) const {
        if (!m_str)
            return 0;
        assert(m_length < INT_MAX);
        assert(index >= 0);
        if ((size_t)index >= m_length)
            return 0;
        buffer[0] = m_str[index];
        if (((unsigned char)buffer[0] >> 3) == 30) { // 11110xxx, 4 bytes
            if ((size_t)index + 3 >= m_length) return -1;
            buffer[1] = m_str[++index];
            buffer[2] = m_str[++index];
            buffer[3] = m_str[++index];
            buffer[4] = 0;
        } else if (((unsigned char)buffer[0] >> 4) == 14) { // 1110xxxx, 3 bytes
            if ((size_t)index + 2 >= m_length) return -1;
            buffer[1] = m_str[++index];
            buffer[2] = m_str[++index];
            buffer[3] = 0;
        } else if (((unsigned char)buffer[0] >> 5) == 6) { // 110xxxxx, 2 bytes
            if ((size_t)index + 1 >= m_length) return -1;
            buffer[1] = m_str[++index];
            buffer[2] = 0;
        } else {
            buffer[1] = 0;
        }
        return index + 1;
    }

protected:
    void grow(const size_t new_capacity) {
        assert(new_capacity >= m_length);
        auto old_str = m_str;
        m_str = new char[new_capacity + 1];
        if (old_str)
            memcpy(m_str, old_str, sizeof(char) * (m_capacity + 1));
        else
            m_str[0] = '\0';
        delete[] old_str;
        m_capacity = new_capacity;
    }

    void grow_at_least(const size_t min_capacity) {
        if (m_capacity >= min_capacity) return;
        if (m_capacity > 0 && min_capacity <= m_capacity * STRING_GROW_FACTOR) {
            grow(m_capacity * STRING_GROW_FACTOR);
        } else {
            grow(min_capacity);
        }
    }

    void increment_successive_char(const char first_char_in_range, const char last_char_in_range, const char prepend_char_to_grow) {
        assert(m_length > 0);
        ssize_t index = m_length - 1;
        char last_char = m_str[index];
        while (last_char == last_char_in_range) {
            m_str[index] = first_char_in_range;
            if ((--index) < 0)
                break;
            last_char = m_str[index];
        }
        if (index == -1) {
            this->prepend_char(prepend_char_to_grow);
        } else {
            m_str[index]++;
        }
    }

    char *m_str { nullptr };
    size_t m_length { 0 };
    size_t m_capacity { 0 };
};
}
