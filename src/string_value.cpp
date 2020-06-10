#include "natalie.hpp"
#include "string.h"

namespace Natalie {

void StringValue::grow(Env *env, ssize_t new_capacity) {
    assert(new_capacity >= m_length);
    m_str = static_cast<char *>(realloc(m_str, new_capacity + 1));
    m_capacity = new_capacity;
}

void StringValue::grow_at_least(Env *env, ssize_t min_capacity) {
    if (m_capacity >= min_capacity) return;
    if (m_capacity > 0 && min_capacity <= m_capacity * STRING_GROW_FACTOR) {
        grow(env, m_capacity * STRING_GROW_FACTOR);
    } else {
        grow(env, min_capacity);
    }
}

void StringValue::append(Env *env, const char *str) {
    ssize_t new_length = strlen(str);
    if (new_length == 0) return;
    ssize_t total_length = m_length + new_length;
    grow_at_least(env, total_length);
    strcat(m_str, str);
    m_length = total_length;
}

void StringValue::append_char(Env *env, char c) {
    ssize_t total_length = m_length + 1;
    grow_at_least(env, total_length);
    m_str[total_length - 1] = c;
    m_str[total_length] = 0;
    m_length = total_length;
}

void StringValue::append_string(Env *env, Value *value) {
    append_string(env, value->as_string());
}

void StringValue::append_string(Env *env, StringValue *string2) {
    if (string2->length() == 0) return;
    ssize_t total_length = m_length + string2->length();
    grow_at_least(env, total_length);
    strncat(m_str, string2->c_str(), string2->length());
    m_length = total_length;
}

// FIXME: I don't like this
#define NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, message_format, ...)                      \
    {                                                                                                 \
        Value *Encoding = NAT_OBJECT->const_get(env, "Encoding", true);                               \
        Value *InvalidByteSequenceError = Encoding->const_get(env, "InvalidByteSequenceError", true); \
        raise(env, InvalidByteSequenceError, message_format, ##__VA_ARGS__);                          \
    }

ArrayValue *StringValue::chars(Env *env) {
    ArrayValue *ary = new ArrayValue { env };
    StringValue *c;
    char buffer[5];
    switch (m_encoding) {
    case Encoding::UTF_8:
        for (ssize_t i = 0; i < m_length; i++) {
            buffer[0] = m_str[i];
            if (((unsigned char)buffer[0] >> 3) == 30) { // 11110xxx, 4 bytes
                if (i + 3 >= m_length) abort(); // NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, send(env, this, "inspect", 0, NULL, NULL));
                buffer[1] = m_str[++i];
                buffer[2] = m_str[++i];
                buffer[3] = m_str[++i];
                buffer[4] = 0;
            } else if (((unsigned char)buffer[0] >> 4) == 14) { // 1110xxxx, 3 bytes
                if (i + 2 >= m_length) abort(); // NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, send(env, this, "inspect", 0, NULL, NULL));
                buffer[1] = m_str[++i];
                buffer[2] = m_str[++i];
                buffer[3] = 0;
            } else if (((unsigned char)buffer[0] >> 5) == 6) { // 110xxxxx, 2 bytes
                if (i + 1 >= m_length) abort(); // NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, send(env, this, "inspect", 0, NULL, NULL));
                buffer[1] = m_str[++i];
                buffer[2] = 0;
            } else {
                buffer[1] = 0;
            }
            c = new StringValue { env, buffer };
            c->set_encoding(Encoding::UTF_8);
            ary->push(c);
        }
        break;
    case Encoding::ASCII_8BIT:
        for (ssize_t i = 0; i < m_length; i++) {
            buffer[0] = m_str[i];
            buffer[1] = 0;
            c = new StringValue { env, buffer };
            c->set_encoding(Encoding::ASCII_8BIT);
            ary->push(c);
        }
        break;
    }
    return ary;
}

SymbolValue *StringValue::to_symbol(Env *env) {
    return SymbolValue::intern(env, m_str);
}

StringValue *StringValue::inspect(Env *env) {
    StringValue *out = new StringValue { env, "\"" };
    for (ssize_t i = 0; i < m_length; i++) {
        char c = m_str[i];
        if (c == '"' || c == '\\' || c == '#') {
            out->append_char(env, '\\');
            out->append_char(env, c);
        } else if (c == '\n') {
            out->append(env, "\\n");
        } else if (c == '\t') {
            out->append(env, "\\t");
        } else {
            out->append_char(env, c);
        }
    }
    out->append_char(env, '"');
    return out;
}

bool StringValue::operator==(const Value &other) const {
    if (!other.is_string()) return false;
    const StringValue *rhs_string = const_cast<Value &>(other).as_string();
    return length() == rhs_string->length() && strcmp(c_str(), rhs_string->c_str()) == 0;
}

StringValue *StringValue::successive(Env *env) {
    StringValue *result = new StringValue { env, m_str };
    ssize_t index = m_length - 1;
    char last_char = m_str[index];
    if (last_char == 'z') {
        result->increment_successive_char(env, 'a', 'a', 'z');
    } else if (last_char == 'Z') {
        result->increment_successive_char(env, 'A', 'A', 'Z');
    } else if (last_char == '9') {
        result->increment_successive_char(env, '0', '1', '9');
    } else {
        char *next = strdup(result->c_str());
        next[index]++;
        result->set_str(next);
        free(next);
    }
    return result;
}

void StringValue::increment_successive_char(Env *env, char append_char, char begin_char, char end_char) {
    ssize_t index = m_length - 1;
    char last_char = m_str[index];
    while (last_char == end_char) {
        m_str[index] = begin_char;
        last_char = m_str[--index];
    }
    if (index == -1) {
        this->append_char(env, append_char);
    } else {
        m_str[index]++;
    }
}

ssize_t StringValue::index(Env *env, StringValue *needle) {
    return index(env, needle, 0);
}

// FIXME: this does not honor multi-byte characters :-(
ssize_t StringValue::index(Env *env, StringValue *needle, ssize_t start) {
    const char *ptr = strstr(c_str() + start, needle->c_str());
    if (ptr == nullptr) {
        return -1;
    }
    return ptr - c_str();
}

StringValue *StringValue::sprintf(Env *env, const char *format, ...) {
    va_list args;
    va_start(args, format);
    StringValue *out = vsprintf(env, format, args);
    va_end(args);
    return out;
}

StringValue *StringValue::vsprintf(Env *env, const char *format, va_list args) {
    StringValue *out = new StringValue { env, "" };
    ssize_t len = strlen(format);
    StringValue *inspected;
    char buf[NAT_INT_64_MAX_BUF_LEN];
    for (ssize_t i = 0; i < len; i++) {
        char c = format[i];
        if (c == '%') {
            char c2 = format[++i];
            switch (c2) {
            case 's':
                out->append(env, va_arg(args, char *));
                break;
            case 'S':
                out->append_string(env, va_arg(args, StringValue *));
                break;
            case 'i':
            case 'd':
                int_to_string(va_arg(args, int64_t), buf);
                out->append(env, buf);
                break;
            case 'x':
                int_to_hex_string(va_arg(args, int64_t), buf, false);
                out->append(env, buf);
                break;
            case 'X':
                int_to_hex_string(va_arg(args, int64_t), buf, true);
                out->append(env, buf);
                break;
            case 'v':
                inspected = send(env, va_arg(args, Value *), "inspect", 0, NULL, NULL)->as_string();
                out->append_string(env, inspected);
                break;
            case '%':
                out->append_char(env, '%');
                break;
            default:
                fprintf(stderr, "Unknown format specifier: %%%c", c2);
                abort();
            }
        } else {
            out->append_char(env, c);
        }
    }
    return out;
}

}
