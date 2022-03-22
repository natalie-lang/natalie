#pragma once

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// from dtoa.c by David Gay
extern "C" char *dtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve);

namespace TM {

class String {
public:
    static constexpr int STRING_GROW_FACTOR = 2;

    String() { }

    String(const char *str) {
        assert(str);
        set_str(str);
    }

    String(const char *str, size_t length) {
        assert(str);
        set_str(str, length);
    }

    String(const String &other) {
        set_str(other.c_str(), other.length());
    }

    String(const String *other) {
        assert(other);
        set_str(other->c_str(), other->length());
    }

    String(char c) {
        char buf[2] = { c, 0 };
        set_str(buf);
    }

    String(size_t length, char c) {
        char buf[length];
        memset(buf, c, sizeof(char) * length);
        set_str(buf, length);
    }

    String(long long number) {
        int length = snprintf(NULL, 0, "%lli", number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%lli", number);
        set_str(buf);
    }

    String(int number) {
        int length = snprintf(NULL, 0, "%d", number);
        char buf[length + 1];
        snprintf(buf, length + 1, "%d", number);
        set_str(buf);
    }

    String(double number, int precision = 4) {
        int length = snprintf(NULL, 0, "%.*f", precision, number);
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

    static String hex(long long number, HexFormat format = HexFormat::UppercaseAndPrefixed) {
        bool uppercase = format == HexFormat::UppercaseAndPrefixed || format == HexFormat::Uppercase;
        bool prefixed = format == HexFormat::UppercaseAndPrefixed || format == HexFormat::LowercaseAndPrefixed;
        const char *format_str = uppercase ? "%llX" : "%llx";
        int length = snprintf(NULL, 0, format_str, number);
        char buf[length + 1];
        snprintf(buf, length + 1, format_str, number);
        auto str = String(buf);
        if (prefixed)
            str.prepend(uppercase ? "0X" : "0x");
        return str;
    }

    virtual ~String() {
        delete[] m_str;
    }

    String &operator=(const String &other) {
        set_str(other.c_str(), other.length());
        return *this;
    }

    String &operator=(const char *other) {
        if (other[0] == '\0') {
            truncate(0);
            return *this;
        }
        set_str(other);
        return *this;
    }

    char at(size_t index) const {
        assert(index < m_length);
        return m_str[index];
    }

    char operator[](size_t index) const {
        return m_str[index];
    }

    char &operator[](size_t index) { return m_str[index]; }

    char last_char() const {
        assert(m_length > 0);
        return m_str[m_length - 1];
    }

    char pop_char() {
        assert(m_length > 0);
        return m_str[--m_length];
    }

    String clone() const { return String { *this }; }

    String substring(size_t start, size_t length) const {
        assert(start < m_length);
        assert(start + length <= m_length);
        return String(c_str() + start, length);
    }

    String substring(size_t start) const {
        return substring(start, m_length - start);
    }

    const char *c_str() const { return m_str ? m_str : ""; }
    size_t bytesize() const { return m_length; }
    size_t length() const { return m_length; }
    size_t size() const { return m_length; }
    size_t capacity() const { return m_capacity; }

    void set_str(const char *str) {
        assert(str);
        auto old_str = m_str;
        m_length = strlen(str);
        m_capacity = m_length;
        m_str = new char[m_length + 1];
        memcpy(m_str, str, sizeof(char) * (m_length + 1));
        if (old_str)
            delete[] old_str;
    }

    void set_str(const char *str, size_t length) {
        assert(str);
        auto old_str = m_str;
        m_str = new char[length + 1];
        memcpy(m_str, str, sizeof(char) * length);
        m_str[length] = 0;
        m_length = length;
        m_capacity = length;
        if (old_str)
            delete[] old_str;
    }

    void prepend_char(char c) {
        size_t total_length = m_length + 1;
        grow_at_least(total_length);
        memmove(m_str + 1, m_str, m_length + 1); // 1 extra for null terminator
        m_str[0] = c;
        m_length = total_length;
    }

    void prepend(long long i) {
        int length = snprintf(NULL, 0, "%lli", i);
        char buf[length + 1];
        snprintf(buf, length + 1, "%lli", i);
        prepend(buf);
    }

    void prepend(const char *str) {
        if (!str) return;
        size_t new_length = strlen(str);
        if (new_length == 0) return;
        char buf[m_length + 1];
        memcpy(buf, c_str(), sizeof(char) * (m_length + 1));
        set_str(str);
        append(buf);
    }

    void prepend(const String &str) {
        size_t new_length = str.length();
        if (new_length == 0) return;
        char buf[new_length + m_length + 1];
        memcpy(buf, str.c_str(), sizeof(char) * new_length);
        memcpy(buf + new_length, c_str(), sizeof(char) * (m_length + 1));
        set_str(buf);
    }

    void insert(size_t position, char c) {
        assert(position < m_length);
        grow_at_least(m_length + 1);
        m_length++;
        size_t nbytes = m_length - position + 1; // 1 extra for null terminator
        memmove(m_str + position + 1, m_str + position, nbytes);
        m_str[position] = c;
    }

    void append_char(char c) {
        size_t total_length = m_length + 1;
        grow_at_least(total_length);
        m_str[total_length - 1] = c;
        m_str[total_length] = 0;
        m_length = total_length;
    }

    void append(signed char c) {
        size_t total_length = m_length + 1;
        grow_at_least(total_length);
        m_str[total_length - 1] = c;
        m_str[total_length] = 0;
        m_length = total_length;
    }

    void append(unsigned char c) {
        append(static_cast<signed char>(c));
    }

    void append(size_t i) {
        int length = snprintf(NULL, 0, "%zu", i);
        char buf[length + 1];
        snprintf(buf, length + 1, "%zu", i);
        append(buf);
    }

    void append(long long i) {
        int length = snprintf(NULL, 0, "%lli", i);
        char buf[length + 1];
        snprintf(buf, length + 1, "%lli", i);
        append(buf);
    }

    void append(int i) {
        int length = snprintf(NULL, 0, "%i", i);
        char buf[length + 1];
        snprintf(buf, length + 1, "%i", i);
        append(buf);
    }

    void append(const char *str) {
        if (!str) return;
        size_t length = strlen(str);
        if (length == 0) return;
        append(str, length);
    }

    void append(const char *str, size_t length) {
        if (!str) return;
        if (length == 0) return;
        size_t total_length = m_length + length;
        grow_at_least(total_length);
        memcpy(m_str + m_length, str, length);
        m_str[total_length] = 0;
        m_length = total_length;
    }

    void append_sprintf(const char *format, ...) {
        va_list args;
        va_start(args, format);
        append_vsprintf(format, args);
        va_end(args);
    }

    void append_vsprintf(const char *format, va_list args) {
        va_list args_copy;
        va_copy(args_copy, args);
        int fmt_length = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);
        char buf[fmt_length + 1];
        vsnprintf(buf, fmt_length + 1, format, args);
        append(buf);
    }

    void append(const String &str) {
        if (str.length() == 0) return;
        size_t total_length = m_length + str.length();
        grow_at_least(total_length);
        memcpy(m_str + m_length, str.c_str(), sizeof(char) * str.length());
        m_length = total_length;
        m_str[m_length] = 0;
    }

    void append(size_t n, char c) {
        size_t total_length = m_length + n;
        grow_at_least(total_length);
        memset(m_str + m_length, c, sizeof(char) * n);
        m_length = total_length;
        m_str[m_length] = 0;
    }

    bool operator==(const String &other) const {
        if (length() != other.length())
            return false;
        return memcmp(c_str(), other.c_str(), sizeof(char) * m_length) == 0;
    }

    bool operator==(const char *other) const {
        assert(other);
        if (length() != strlen(other))
            return false;
        return memcmp(c_str(), other, sizeof(char) * m_length) == 0;
    }

    bool operator!=(const String &other) const {
        return !operator==(other);
    }

    bool operator>(const String &other) const {
        // FIXME: cannot use strcmp here
        return strcmp(c_str(), other.c_str()) > 0;
    }

    bool operator<(const String &other) const {
        // FIXME: cannot use strcmp here
        return strcmp(c_str(), other.c_str()) < 0;
    }

    int cmp(const String &other) const {
        if (m_length == 0) {
            if (other.m_length == 0)
                return 0;
            return -1;
        }
        size_t i;
        for (i = 0; i < std::min(m_length, other.m_length); ++i) {
            auto c1 = (unsigned char)(*this)[i], c2 = (unsigned char)other[i];
            if (c1 < c2)
                return -1;
            else if (c1 > c2)
                return 1;
        }
        // "x" (len 1) <=> "xx" (len 2)
        // 1 - 2 = -1
        return m_length - other.m_length;
    }

    ssize_t find(const String &needle) const {
        if (m_length < needle.length() || needle.is_empty())
            return -1;
        assert(m_str);
        size_t max_index = m_length - needle.length();
        size_t byte_count = sizeof(char) * needle.length();
        for (size_t index = 0; index <= max_index; ++index) {
            if (memcmp(m_str + index, needle.c_str(), byte_count) == 0)
                return index;
        }
        return -1;
    }

    ssize_t find(const char c) const {
        for (size_t i = 0; i < m_length; ++i) {
            if (c == m_str[i]) return i;
        }
        return -1;
    }

    void truncate(size_t length) {
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

    void clear() { truncate(0); }

    void chomp() {
        if (m_length == 0) return;
        truncate(m_length - 1);
    }

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

    void strip_trailing_spaces() {
        while (m_length > 0) {
            if (m_str[m_length - 1] == ' ')
                chomp();
            else
                return;
        }
    }

    void remove(char character) {
        size_t i;
        assert(m_str);
        for (i = 0; i < m_length; ++i) {
            if (m_str[i] == character) {
                for (size_t j = i; j < m_length; ++j)
                    m_str[j] = m_str[j + 1];

                --m_length;
                --i;
            }
        }
        m_str[m_length] = '\0';
    }

    bool is_empty() const { return m_length == 0; }

    String successive() {
        auto result = String { *this };
        assert(m_length > 0);
        size_t index = length() - 1;
        char last_char = m_str[index];
        if (last_char == 'z') {
            result.increment_successive_char('a', 'a', 'z');
        } else if (last_char == 'Z') {
            result.increment_successive_char('A', 'A', 'Z');
        } else if (last_char == '9') {
            result.increment_successive_char('0', '1', '9');
        } else {
            result.m_str[index]++;
        }
        return result;
    }

    template <typename... Args>
    static String format(const char *fmt, Args... args) {
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
                c++;
                out.append(first);
                format(out, c + 1, rest...);
                return;
            } else {
                out.append_char(*c);
            }
        }
    }

    String uppercase() const {
        auto new_str = new String(this);
        for (size_t i = 0; i < new_str->m_length; ++i) {
            new_str->m_str[i] = toupper(new_str->m_str[i]);
        }
        return new_str;
    }

    String lowercase() const {
        auto new_str = new String(this);
        for (size_t i = 0; i < new_str->m_length; ++i) {
            new_str->m_str[i] = tolower(new_str->m_str[i]);
        }
        return new_str;
    }

    bool ends_with(const String &needle) {
        if (m_length < needle.m_length)
            return false;
        return memcmp(m_str + m_length - needle.m_length, needle.m_str, needle.m_length) == 0;
    }

    // djb2 hash algorithm by Dan Bernstein
    size_t djb2_hash() const {
        size_t hash = 5381;
        int c;
        for (size_t i = 0; i < m_length; ++i) {
            c = (*this)[i];
            hash = ((hash << 5) + hash) + c;
        }
        return hash;
    }

    void print() const {
        for (size_t i = 0; i < m_length; ++i) {
            printf("%c", (*this)[i]);
        }
        printf("\n");
    }

private:
    void grow(size_t new_capacity) {
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

    void grow_at_least(size_t min_capacity) {
        if (m_capacity >= min_capacity) return;
        if (m_capacity > 0 && min_capacity <= m_capacity * STRING_GROW_FACTOR) {
            grow(m_capacity * STRING_GROW_FACTOR);
        } else {
            grow(min_capacity);
        }
    }

    void increment_successive_char(char append, char begin_char, char end_char) {
        assert(m_length > 0);
        ssize_t index = m_length - 1;
        char last_char = m_str[index];
        while (last_char == end_char) {
            m_str[index] = begin_char;
            last_char = m_str[--index];
        }
        if (index == -1) {
            this->append_char(append);
        } else {
            m_str[index]++;
        }
    }

    char *m_str { nullptr };
    size_t m_length { 0 };
    size_t m_capacity { 0 };
};
}
