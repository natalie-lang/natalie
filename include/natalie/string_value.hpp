#pragma once

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "natalie/class_value.hpp"
#include "natalie/encoding_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct StringValue : Value {
    const int STRING_GROW_FACTOR = 2;

    StringValue(Env *env, ClassValue *klass)
        : Value { Value::Type::String, klass } {
        m_str = strdup("");
    }

    StringValue(Env *env)
        : StringValue { env, "" } { }

    StringValue(Env *env, const char *str)
        : Value { Value::Type::String, NAT_OBJECT->const_get(env, "String", true)->as_class() } {
        set_str(str);
    }

    StringValue(Env *env, const char *str, ssize_t length)
        : Value { Value::Type::String, NAT_OBJECT->const_get(env, "String", true)->as_class() } {
        set_str(str, length);
    }

    StringValue(const StringValue &other)
        : Value { Value::Type::String, other.klass } {
        set_str(other.c_str(), other.length());
    }

    static StringValue *sprintf(Env *, const char *, ...);
    static StringValue *vsprintf(Env *, const char *, va_list);

    const char *c_str() const { return m_str; }
    ssize_t length() const { return m_length; }
    ssize_t capcity() const { return m_capacity; }
    Encoding encoding() const { return m_encoding; }

    void set_str(const char *str) {
        free(m_str);
        assert(str);
        m_str = strdup(str);
        m_length = strlen(str);
        m_capacity = m_length;
    }

    void set_str(const char *str, ssize_t length) {
        free(m_str);
        assert(str);
        m_str = strdup(str);
        m_str[length] = 0;
        m_length = length;
        m_capacity = strlen(str);
        assert(m_length <= m_capacity);
    }

    void set_encoding(Encoding encoding) { m_encoding = encoding; }

    void append(Env *, const char *);
    void append_char(Env *, char);
    void append_string(Env *, Value *);
    void append_string(Env *, StringValue *);
    ArrayValue *chars(Env *);

    SymbolValue *to_symbol(Env *);

    StringValue *to_str() { return this; }

    StringValue *inspect(Env *);

    bool operator==(const Value &) const;

    StringValue *successive(Env *);

    Value *index(Env *, Value *);
    Value *index(Env *, Value *, ssize_t start);
    ssize_t index_ssize_t(Env *, Value *, ssize_t start);

    void truncate(ssize_t length) {
        assert(length <= m_length);
        m_str[length] = 0;
        m_length = length;
    }

    Value *initialize(Env *, Value *);
    Value *ltlt(Env *, Value *);

    bool eq(Value *arg) {
        return *this == *arg;
    }

    Value *to_s() {
        return this;
    }

    Value *add(Env *, Value *);
    Value *mul(Env *, Value *);
    Value *cmp(Env *, Value *);
    Value *eqtilde(Env *, Value *);
    Value *match(Env *, Value *);
    Value *ord(Env *);
    Value *bytes(Env *);
    Value *size(Env *);
    Value *encoding(Env *);
    Value *encode(Env *, Value *);
    Value *force_encoding(Env *, Value *);
    Value *ref(Env *, Value *);
    Value *sub(Env *, Value *, Value *);
    Value *to_i(Env *, Value *);
    Value *split(Env *, Value *);
    Value *ljust(Env *, Value *, Value *);

private:
    using Value::Value;

    void grow(Env *, ssize_t);
    void grow_at_least(Env *, ssize_t);

    void increment_successive_char(Env *, char, char, char);

    void raise_encoding_invalid_byte_sequence_error(Env *, ssize_t);

    char *m_str { nullptr };
    ssize_t m_length { 0 };
    ssize_t m_capacity { 0 };
    Encoding m_encoding { Encoding::UTF_8 };
};
}
