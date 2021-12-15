#pragma once

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "natalie/class_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/string.hpp"

namespace Natalie {

class StringObject : public Object {
public:
    const int STRING_GROW_FACTOR = 2;

    StringObject(ClassObject *klass)
        : Object { Object::Type::String, klass } { }

    StringObject()
        : StringObject { "" } { }

    StringObject(const char *str)
        : Object { Object::Type::String, GlobalEnv::the()->String() } {
        set_str(str);
    }

    StringObject(const char *str, Encoding encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        set_str(str);
    }

    StringObject(const char *str, size_t length)
        : Object { Object::Type::String, GlobalEnv::the()->String() } {
        set_str(str, length);
    }

    StringObject(const StringObject &other)
        : Object { other } {
        set_str(other.c_str(), other.length());
    }

    StringObject(const String &str)
        : Object { Object::Type::String, GlobalEnv::the()->String() } {
        m_string = str;
    }

    StringObject(const String &str, Encoding encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        m_string = str;
    }

    String *to_low_level_string() const { return m_string.clone(); }
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

    void append_char(char);
    void append(signed char);
    void append(unsigned char c) { append(static_cast<signed char>(c)); }
    void append(Env *, const char *);
    void append(Env *, const StringObject *);
    void append(Env *, const String *);
    void append(Env *, ValuePtr);

    void append_sprintf(const char *format, ...) {
        va_list args, args_copy;
        va_start(args, format);
        m_string.append_vsprintf(format, args);
        va_end(args);
    }

    char *next_char(Env *, char *, size_t *);
    ValuePtr each_char(Env *, Block *);
    ArrayObject *chars(Env *);

    SymbolObject *to_symbol(Env *) const;
    ValuePtr to_sym(Env *) const;

    StringObject *to_str() { return this; }

    StringObject *inspect(Env *);

    bool operator==(const Object &value) const {
        if (!value.is_string())
            return false;
        return *this == *value.as_string();
    }

    bool operator==(const StringObject &value) const {
        return m_string == value.m_string;
    }

    StringObject *successive(Env *);

    ValuePtr index(Env *, ValuePtr);
    ValuePtr index(Env *, ValuePtr, size_t start);
    nat_int_t index_int(Env *, ValuePtr, size_t start) const;

    void truncate(size_t length) {
        m_string.truncate(length);
    }

    ValuePtr initialize(Env *, ValuePtr);
    ValuePtr ltlt(Env *, ValuePtr);

    bool eql(ValuePtr arg) const {
        return *this == *arg;
    }

    ValuePtr to_s() {
        return this;
    }

    bool start_with(Env *, ValuePtr) const;
    bool end_with(Env *, ValuePtr) const;
    bool is_empty() const { return m_string.is_empty(); }

    ValuePtr gsub(Env *, ValuePtr, ValuePtr = nullptr, Block *block = nullptr);
    ValuePtr sub(Env *, ValuePtr, ValuePtr = nullptr, Block *block = nullptr);

    ValuePtr add(Env *, ValuePtr) const;
    ValuePtr bytes(Env *) const;
    ValuePtr cmp(Env *, ValuePtr) const;
    ValuePtr downcase(Env *);
    ValuePtr encode(Env *, ValuePtr);
    ValuePtr encoding(Env *);
    bool eq(Env *, ValuePtr arg);
    ValuePtr eqtilde(Env *, ValuePtr);
    ValuePtr force_encoding(Env *, ValuePtr);
    ValuePtr ljust(Env *, ValuePtr, ValuePtr);
    ValuePtr lstrip(Env *) const;
    ValuePtr match(Env *, ValuePtr);
    ValuePtr mul(Env *, ValuePtr) const;
    ValuePtr ord(Env *);
    ValuePtr ref(Env *, ValuePtr);
    ValuePtr reverse(Env *);
    ValuePtr rstrip(Env *) const;
    ValuePtr size(Env *);
    ValuePtr split(Env *, ValuePtr, ValuePtr);
    ValuePtr strip(Env *) const;
    ValuePtr to_i(Env *, ValuePtr) const;
    ValuePtr upcase(Env *);

    template <typename... Args>
    static StringObject *format(Env *env, const char *fmt, Args... args) {
        auto str = String::format(fmt, args...);
        return new StringObject { *str };
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<StringObject %p str='%s'>", this, m_string.c_str());
    }

private:
    StringObject *expand_backrefs(Env *, StringObject *, MatchDataObject *);
    StringObject *regexp_sub(Env *, RegexpObject *, StringObject *, MatchDataObject **, StringObject **, size_t = 0);

    using Object::Object;

    void raise_encoding_invalid_byte_sequence_error(Env *, size_t) const;

    String m_string {};
    Encoding m_encoding { Encoding::UTF_8 };
};
}
