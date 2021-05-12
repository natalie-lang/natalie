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
#include "natalie/string.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct StringValue : Value {
    const int STRING_GROW_FACTOR = 2;

    StringValue(Env *env, ClassValue *klass)
        : Value { Value::Type::String, klass } { }

    StringValue(Env *env)
        : StringValue { env, "" } { }

    StringValue(Env *env, const char *str)
        : Value { Value::Type::String, env->String() } {
        set_str(str);
    }

    StringValue(Env *env, const char *str, Encoding encoding)
        : Value { Value::Type::String, env->String() }
        , m_encoding { encoding } {
        set_str(str);
    }

    StringValue(Env *env, const char *str, size_t length)
        : Value { Value::Type::String, env->String() } {
        set_str(str, length);
    }

    StringValue(Env *env, const StringValue &other)
        : Value { env, other } {
        set_str(other.c_str(), other.length());
    }

    StringValue(Env *env, const String &str)
        : Value { Value::Type::String, env->String() } {
        m_string = str;
    }

    String *to_string() const { return m_string.clone(); }
    const char *c_str() const { return m_string.c_str(); }
    size_t bytesize() const { return m_string.length(); }
    size_t length() const { return m_string.length(); }
    size_t capacity() const { return m_string.capacity(); }
    Encoding encoding() const { return m_encoding; }

    void set_str(const char *str) {
        m_string.set_str(str);
    }

    void set_str(const char *str, size_t length) {
        m_string.set_str(str, length);
    }

    void set_encoding(Encoding encoding) { m_encoding = encoding; }

    void prepend_char(Env *, char);

    void insert(Env *, size_t, char);

    void append_char(Env *, char);
    void append(Env *, const char *);
    void append(Env *, const StringValue *);
    void append(Env *, ValuePtr);

    char *next_char(Env *, char *, size_t *);
    ValuePtr each_char(Env *, Block *);
    ArrayValue *chars(Env *);

    SymbolValue *to_symbol(Env *);
    ValuePtr to_sym(Env *);

    StringValue *to_str() { return this; }

    StringValue *inspect(Env *);

    bool operator==(const Value &value) const {
        if (!value.is_string())
            return false;
        return *this == *const_cast<Value &>(value).as_string(); // FIXME: const
    }

    bool operator==(const StringValue &value) const {
        return m_string == value.m_string;
    }

    StringValue *successive(Env *);

    ValuePtr index(Env *, ValuePtr);
    ValuePtr index(Env *, ValuePtr, size_t start);
    nat_int_t index_int(Env *, ValuePtr, size_t start);

    void truncate(size_t length) {
        m_string.truncate(length);
    }

    ValuePtr initialize(Env *, ValuePtr);
    ValuePtr ltlt(Env *, ValuePtr);

    bool eq(ValuePtr arg) {
        return *this == *arg;
    }

    ValuePtr to_s() {
        return this;
    }

    bool start_with(Env *, ValuePtr);
    bool end_with(Env *, ValuePtr);
    bool is_empty() { return m_string.is_empty(); }

    ValuePtr gsub(Env *, ValuePtr, ValuePtr = nullptr, Block *block = nullptr);
    ValuePtr sub(Env *, ValuePtr, ValuePtr = nullptr, Block *block = nullptr);

    ValuePtr add(Env *, ValuePtr);
    ValuePtr bytes(Env *);
    ValuePtr cmp(Env *, ValuePtr);
    ValuePtr downcase(Env *);
    ValuePtr encode(Env *, ValuePtr);
    ValuePtr encoding(Env *);
    ValuePtr eqtilde(Env *, ValuePtr);
    ValuePtr force_encoding(Env *, ValuePtr);
    ValuePtr ljust(Env *, ValuePtr, ValuePtr);
    ValuePtr match(Env *, ValuePtr);
    ValuePtr mul(Env *, ValuePtr);
    ValuePtr ord(Env *);
    ValuePtr ref(Env *, ValuePtr);
    ValuePtr reverse(Env *);
    ValuePtr size(Env *);
    ValuePtr split(Env *, ValuePtr, ValuePtr);
    ValuePtr strip(Env *);
    ValuePtr to_i(Env *, ValuePtr);
    ValuePtr upcase(Env *);

    template <typename... Args>
    static StringValue *format(Env *env, const char *fmt, Args... args) {
        auto str = String::format(fmt, args...);
        return new StringValue { env, *str };
    }

private:
    StringValue *expand_backrefs(Env *, StringValue *, MatchDataValue *);
    StringValue *regexp_sub(Env *, RegexpValue *, StringValue *, MatchDataValue **, StringValue **, size_t = 0);

    using Value::Value;

    void raise_encoding_invalid_byte_sequence_error(Env *, size_t);

    String m_string {};
    Encoding m_encoding { Encoding::UTF_8 };
};
}
