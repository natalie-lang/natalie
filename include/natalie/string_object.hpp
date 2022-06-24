#pragma once

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "natalie/class_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/managed_string.hpp"
#include "natalie/object.hpp"

namespace Natalie {

using namespace TM;

class StringObject : public Object {
public:
    const int STRING_GROW_FACTOR = 2;

    StringObject(ClassObject *klass)
        : Object { Object::Type::String, klass }
        , m_encoding { Encoding::ASCII_8BIT } { }

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

    ManagedString *to_low_level_string() const { return new ManagedString(m_string); }
    const String &string() const { return m_string; }

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
    void append(Env *, const ManagedString *);
    void append(Env *, const String &);
    void append(Env *, Value);

    void append_sprintf(const char *format, ...) {
        va_list args, args_copy;
        va_start(args, format);
        m_string.append_vsprintf(format, args);
        va_end(args);
    }

    char *next_char(Env *, char *, size_t *);
    Value each_char(Env *, Block *);
    ArrayObject *chars(Env *);

    SymbolObject *to_symbol(Env *) const;
    Value to_sym(Env *) const;

    StringObject *to_str() { return this; }

    StringObject *inspect(Env *);

    StringObject &operator=(StringObject other) {
        this->m_string = other.m_string;
        this->m_encoding = other.m_encoding;
        return *this;
    }

    bool operator==(const Object &value) const {
        if (!value.is_string())
            return false;
        return *this == *value.as_string();
    }

    bool operator==(const StringObject &value) const {
        return m_string == value.m_string;
    }

    StringObject *successive(Env *);

    Value index(Env *, Value);
    Value index(Env *, Value, size_t start);
    nat_int_t index_int(Env *, Value, size_t start) const;

    void truncate(size_t length) {
        m_string.truncate(length);
    }

    Value initialize(Env *, Value);
    Value initialize_copy(Env *, Value);
    Value ltlt(Env *, Value);

    bool eql(Value arg) const {
        return *this == *arg;
    }

    Value to_s() {
        return this;
    }

    bool internal_start_with(Env *, Value) const;
    bool start_with(Env *, Args) const;
    bool end_with(Env *, Value) const;
    bool is_empty() const { return m_string.is_empty(); }

    Value gsub(Env *, Value, Value = nullptr, Block *block = nullptr);
    Value sub(Env *, Value, Value = nullptr, Block *block = nullptr);

    Value add(Env *, Value) const;
    Value b(Env *) const;
    Value bytes(Env *, Block *);
    Value center(Env *, Value, Value);
    Value chr(Env *);
    Value chomp(Env *, Value);
    Value clear(Env *);
    Value cmp(Env *, Value) const;
    Value concat(Env *env, Args args);
    Value downcase(Env *);
    Value each_byte(Env *, Block *);
    Value encode(Env *, Value);
    Value encoding(Env *);
    bool eq(Env *, Value arg);
    Value eqtilde(Env *, Value);
    Value force_encoding(Env *, Value);
    bool include(Env *, Value);
    bool include(const char *) const;
    Value ljust(Env *, Value, Value);
    Value lstrip(Env *) const;
    Value lstrip_in_place(Env *);
    Value match(Env *, Value);
    Value mul(Env *, Value) const;
    Value ord(Env *);
    Value prepend(Env *, Args);
    Value ref(Env *, Value);
    Value refeq(Env *, Value, Value, Value);
    Value reverse(Env *);
    Value reverse_in_place(Env *);
    Value rstrip(Env *) const;
    Value rstrip_in_place(Env *);
    Value size(Env *);
    Value split(Env *, Value, Value);
    Value strip(Env *) const;
    Value strip_in_place(Env *);
    Value to_i(Env *, Value) const;
    Value upcase(Env *);
    Value uplus(Env *);

    Value convert_float();

    static size_t byte_index_to_char_index(ArrayObject *chars, size_t byte_index);

    template <typename... Args>
    static StringObject *format(const char *fmt, Args... args) {
        String out;
        format(out, fmt, args...);
        return new StringObject { out };
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
                if constexpr (std::is_same<T, const StringObject *>::value || std::is_same<T, StringObject *>::value)
                    out.append(first->to_low_level_string());
                else
                    out.append(first);
                format(out, c + 1, rest...);
                return;
            } else {
                out.append_char(*c);
            }
        }
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<StringObject %p str='%s'>", this, m_string.c_str());
    }

private:
    StringObject *expand_backrefs(Env *, StringObject *, MatchDataObject *);
    StringObject *regexp_sub(Env *, RegexpObject *, StringObject *, MatchDataObject **, StringObject **, size_t = 0, Block *block = nullptr);

    using Object::Object;

    void raise_encoding_invalid_byte_sequence_error(Env *, size_t) const;

    String m_string {};
    Encoding m_encoding { Encoding::UTF_8 };
};
}
