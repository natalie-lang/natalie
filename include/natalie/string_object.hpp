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
        , m_encoding { EncodingObject::get(Encoding::ASCII_8BIT) } {
        assert(m_encoding);
    }

    StringObject()
        : StringObject { "" } { }

    StringObject(const char *str)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(Encoding::UTF_8) } {
        assert(m_encoding);
        set_str(str);
    }

    StringObject(const char *str, EncodingObject *encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        set_str(str);
    }

    StringObject(const char *str, size_t length)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(Encoding::UTF_8) } {
        assert(m_encoding);
        set_str(str, length);
    }

    StringObject(const StringObject &other)
        : Object { other } {
        set_str(other.c_str(), other.length());
        set_encoding(other.encoding());
    }

    StringObject(const String &str)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(Encoding::UTF_8) } {
        assert(m_encoding);
        m_string = str;
    }

    StringObject(const String &str, EncodingObject *encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        m_string = str;
    }

    StringObject(const StringView &str, EncodingObject *encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        m_string = str.clone();
    }

    ManagedString *to_low_level_string() const { return new ManagedString(m_string); }
    const String &string() const { return m_string; }

    const char *c_str() const { return m_string.c_str(); }
    size_t bytesize() const { return m_string.length(); }
    size_t length() const { return m_string.length(); }
    size_t capacity() const { return m_string.capacity(); }

    void set_str(const char *str) {
        m_string.set_str(str);
    }

    void set_str(const char *str, size_t length) {
        m_string.set_str(str, length);
    }

    bool valid_encoding() const;
    EncodingObject *encoding() const { return m_encoding; }
    void set_encoding(EncodingObject *encoding) { m_encoding = encoding; }

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

    StringView prev_char(size_t *) const;
    StringView next_char(size_t *) const;
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

    Value index(Env *, Value) const;
    Value index(Env *, Value, size_t start) const;
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
    bool ascii_only(Env *) const;
    Value b(Env *) const;
    Value bytes(Env *, Block *);
    Value center(Env *, Value, Value);
    Value chr(Env *);
    Value chomp(Env *, Value);
    Value chop(Env *);
    Value chop_in_place(Env *);
    Value clear(Env *);
    Value cmp(Env *, Value) const;
    Value concat(Env *env, Args args);
    Value delete_prefix(Env *, Value);
    Value delete_prefix_in_place(Env *, Value);
    Value delete_suffix(Env *, Value);
    Value delete_suffix_in_place(Env *, Value);
    Value downcase(Env *);
    Value each_byte(Env *, Block *);
    Value encode(Env *, Value);
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
    Value ord(Env *) const;
    Value prepend(Env *, Args);
    Value ref(Env *, Value);
    Value ref_fast_index(Env *, size_t) const;
    Value ref_fast_range(Env *, size_t, size_t) const;
    Value ref_fast_range_endless(Env *, size_t) const;
    Value refeq(Env *, Value, Value, Value);
    Value reverse(Env *);
    Value reverse_in_place(Env *);
    Value rstrip(Env *) const;
    Value rstrip_in_place(Env *);
    Value size(Env *) const;
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

    class iterator {
    public:
        iterator(const StringObject *string, StringView view)
            : m_string { string }
            , m_view { view } { }

        iterator operator++() {
            size_t byte_index = m_view.offset() + m_view.length();
            m_view = m_string->next_char(&byte_index);
            return *this;
        }

        iterator operator++(int) {
            iterator i = *this;
            ++(*this);
            return i;
        }

        StringView operator*() const {
            return m_view;
        }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_string == i2.m_string && i1.m_view == i2.m_view;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_string != i2.m_string || i1.m_view != i2.m_view;
        }

    private:
        const StringObject *m_string;
        StringView m_view;
    };

    iterator begin() const {
        size_t index = 0;
        auto view = next_char(&index);
        return iterator { this, view };
    }

    iterator end() const {
        return iterator { this, StringView(&m_string, m_string.length(), 0) };
    }

private:
    StringObject *expand_backrefs(Env *, StringObject *, MatchDataObject *);
    StringObject *regexp_sub(Env *, RegexpObject *, StringObject *, MatchDataObject **, StringObject **, size_t = 0, Block *block = nullptr);

    using Object::Object;

    String m_string {};
    EncodingObject *m_encoding { nullptr };
};
}
