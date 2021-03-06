#pragma once

#include <assert.h>
#include <string>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {

struct String : public gc {
    const int STRING_GROW_FACTOR = 2;

    String() {
        set_str("");
    }

    String(const std::string str) {
        set_str(str.c_str());
    }

    String(const char *str) {
        set_str(str);
    }

    String(const char *str, size_t length) {
        set_str(str, length);
    }

    String(const String &other) {
        set_str(other.c_str(), other.length());
    }

    String &operator=(const String &other) {
        set_str(other.c_str(), other.length());
        return *this;
    }

    const char *c_str() const { return m_str; }
    size_t bytesize() const { return m_length; }
    size_t length() const { return m_length; }
    size_t capacity() const { return m_capacity; }

    void set_str(const char *str) {
        assert(str);
        m_str = GC_STRDUP(str);
        m_length = strlen(str);
        m_capacity = m_length;
    }

    void set_str(const char *str, size_t length) {
        assert(str);
        m_str = new char[length + 1];
        assert(strlen(str) >= length);
        strncpy(m_str, str, length);
        m_str[length] = 0;
        m_length = length;
        m_capacity = length;
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

    void append(char c) {
        size_t total_length = m_length + 1;
        grow_at_least(total_length);
        m_str[total_length - 1] = c;
        m_str[total_length] = 0;
        m_length = total_length;
    }

    void append(const char *str) {
        if (str == nullptr) return;
        size_t new_length = strlen(str);
        if (new_length == 0) return;
        size_t total_length = m_length + new_length;
        grow_at_least(total_length);
        strcat(m_str, str);
        m_length = total_length;
    }

    void append(const std::string str) {
        size_t new_length = str.length();
        if (new_length == 0) return;
        size_t total_length = m_length + new_length;
        grow_at_least(total_length);
        strcat(m_str, str.c_str());
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

    void truncate(size_t length) {
        assert(length <= m_length);
        m_str[length] = 0;
        m_length = length;
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
            out->append(*c);
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
                out->append(*c);
            }
        }
    }

private:
    void grow(size_t new_capacity) {
        assert(new_capacity >= m_length);
        m_str = static_cast<char *>(GC_REALLOC(m_str, new_capacity + 1));
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
            this->append(append);
        } else {
            m_str[index]++;
        }
    }

    char *m_str { nullptr };
    size_t m_length { 0 };
    size_t m_capacity { 0 };
};
}
