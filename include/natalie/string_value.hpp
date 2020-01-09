#pragma once

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <string>

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
        m_str = GC_STRDUP("");
    }

    StringValue(Env *env)
        : StringValue { env, "" } { }

    StringValue(Env *env, std::string str)
        : Value { Value::Type::String, env->Object()->const_fetch("String")->as_class() } {
        set_str(str.c_str());
    }

    StringValue(Env *env, const char *str)
        : Value { Value::Type::String, env->Object()->const_fetch("String")->as_class() } {
        set_str(str);
    }

    StringValue(Env *env, const char *str, size_t length)
        : Value { Value::Type::String, env->Object()->const_fetch("String")->as_class() } {
        set_str(str, length);
    }

    StringValue(StringValue &other)
        : Value { other } {
        set_str(other.c_str(), other.length());
    }

    static StringValue *sprintf(Env *, const char *, ...);
    static StringValue *vsprintf(Env *, const char *, va_list);

    const char *c_str() const { return m_str; }
    size_t bytesize() const { return m_length; }
    size_t length() const { return m_length; }
    size_t capcity() const { return m_capacity; }
    Encoding encoding() const { return m_encoding; }

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

    void set_encoding(Encoding encoding) { m_encoding = encoding; }

    void prepend_char(Env *, char);

    void insert(Env *, size_t, char);

    void append(Env *, const char *);
    void append(Env *, std::string);
    void append_char(Env *, char);
    void append_string(Env *, Value *);
    void append_string(Env *, StringValue *);
    StringValue *next_char(Env *, size_t *);
    Value *each_char(Env *, Block *);
    ArrayValue *chars(Env *);

    SymbolValue *to_symbol(Env *);
    Value *to_sym(Env *);

    StringValue *to_str() { return this; }

    StringValue *inspect(Env *);

    bool operator==(const Value &) const;

    StringValue *successive(Env *);

    Value *index(Env *, Value *);
    Value *index(Env *, Value *, size_t start);
    nat_int_t index_int(Env *, Value *, size_t start);

    void truncate(size_t length) {
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

    bool start_with(Env *, Value *);
    bool end_with(Env *, Value *);
    bool is_empty() { return m_length == 0; }

    StringValue *gsub(Env *, Value *, Value *);
    StringValue *sub(Env *, Value *, Value *);

    Value *add(Env *, Value *);
    Value *bytes(Env *);
    Value *cmp(Env *, Value *);
    Value *downcase(Env *);
    Value *encode(Env *, Value *);
    Value *encoding(Env *);
    Value *eqtilde(Env *, Value *);
    Value *force_encoding(Env *, Value *);
    Value *ljust(Env *, Value *, Value *);
    Value *match(Env *, Value *);
    Value *mul(Env *, Value *);
    Value *ord(Env *);
    Value *ref(Env *, Value *);
    Value *size(Env *);
    Value *split(Env *, Value *, Value *);
    Value *strip(Env *);
    Value *to_i(Env *, Value *);
    Value *upcase(Env *);

private:
    StringValue *expand_backrefs(Env *, StringValue *, MatchDataValue *);
    StringValue *regexp_sub(Env *, RegexpValue *, StringValue *, MatchDataValue **, StringValue **, size_t = 0);

    using Value::Value;

    void grow(Env *, size_t);
    void grow_at_least(Env *, size_t);

    void increment_successive_char(Env *, char, char, char);

    void raise_encoding_invalid_byte_sequence_error(Env *, size_t);

    char *m_str { nullptr };
    size_t m_length { 0 };
    size_t m_capacity { 0 };
    Encoding m_encoding { Encoding::UTF_8 };
};
}
