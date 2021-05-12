#pragma once

#include <assert.h>
#include <string.h>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {

struct String : public Cell {
    const int STRING_GROW_FACTOR = 2;

    String() {
        set_str("");
    }

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

    virtual ~String() override {
        delete[] m_str;
    }

    String &operator=(const String &other) {
        set_str(other.c_str(), other.length());
        return *this;
    }

    char at(size_t index) const {
        assert(index < m_length);
        return m_str[index];
    }

    char last_char() const {
        assert(m_length > 0);
        return m_str[m_length - 1];
    }

    String *clone() const { return new String { *this }; }

    String *substring(size_t start, size_t length) const {
        assert(start < m_length);
        assert(start + length <= m_length);
        return new String(c_str() + start, length);
    }

    const char *c_str() const { return m_str; }
    size_t bytesize() const { return m_length; }
    size_t length() const { return m_length; }
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
        assert(strlen(str) >= length);
        strncpy(m_str, str, length);
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

    void append(const char *str) {
        if (str == nullptr) return;
        size_t new_length = strlen(str);
        if (new_length == 0) return;
        size_t total_length = m_length + new_length;
        grow_at_least(total_length);
        strncat(m_str, str, new_length);
        m_length = total_length;
    }

    void append(const String *string2) {
        if (string2->length() == 0) return;
        size_t total_length = m_length + string2->length();
        grow_at_least(total_length);
        strncat(m_str, string2->c_str(), string2->length());
        m_length = total_length;
    }

    void append(const StringValue *string2);

    bool operator==(const String &other) const {
        return length() == other.length() && strncmp(c_str(), other.c_str(), m_length) == 0;
    }

    bool operator==(const char *other) const {
        assert(other);
        return length() == strlen(other) && strncmp(c_str(), other, m_length) == 0;
    }

    ssize_t find(const String *needle) const {
        const char *index = strstr(m_str, needle->c_str());
        if (index == nullptr)
            return -1;
        return index - m_str;
    }

    void truncate(size_t length) {
        assert(length <= m_length);
        m_str[length] = 0;
        m_length = length;
    }

    void clear() { truncate(0); }

    void chomp() { truncate(m_length - 1); }

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

    bool is_empty() { return m_length == 0; }

    String successive() {
        auto result = String { *this };
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
    static String *format(const char *fmt, Args... args) {
        auto out = new String {};
        format(out, fmt, args...);
        return out;
    }

    static void format(String *out, const char *fmt) {
        for (const char *c = fmt; *c != 0; c++) {
            out->append_char(*c);
        }
    }

    template <typename T, typename... Args>
    static void format(String *out, const char *fmt, T first, Args... rest) {
        for (const char *c = fmt; *c != 0; c++) {
            if (*c == '{' && *(c + 1) == '}') {
                c++;
                out->append(first);
                format(out, c + 1, rest...);
                return;
            } else {
                out->append_char(*c);
            }
        }
    }

    virtual void visit_children(Visitor &visitor) override final {
    }

private:
    void grow(size_t new_capacity) {
        assert(new_capacity >= m_length);
        auto old_str = m_str;
        m_str = new char[new_capacity + 1];
        memcpy(m_str, old_str, sizeof(char) * (m_capacity + 1));
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
