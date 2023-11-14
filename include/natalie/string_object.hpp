#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "natalie/class_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"

namespace Natalie {

using namespace TM;

enum CaseFoldType {
    Ascii = 1,
    FoldTurkicAzeri = 2,
    FoldLithuanian = 4,
    Upcase = 8,
    Downcase = 16,
    Fold = 32
};

inline CaseFoldType operator|(CaseFoldType a, CaseFoldType b) {
    return static_cast<CaseFoldType>(static_cast<int>(a) | static_cast<int>(b));
}
inline CaseFoldType operator^(CaseFoldType a, CaseFoldType b) {
    return static_cast<CaseFoldType>(static_cast<int>(a) ^ static_cast<int>(b));
}

class StringObject : public Object {
public:
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

    StringObject(const char *str, size_t length, EncodingObject *encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
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

    StringObject(String &&str)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(Encoding::UTF_8) } {
        assert(m_encoding);
        m_string = std::move(str);
    }

    StringObject(const String &str, EncodingObject *encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        m_string = str;
    }

    StringObject(String &&str, EncodingObject *encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        m_string = std::move(str);
    }

    StringObject(const String &str, Encoding encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(encoding) } {
        assert(m_encoding);
        m_string = str;
    }

    StringObject(String &&str, Encoding encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(encoding) } {
        assert(m_encoding);
        m_string = std::move(str);
    }

    StringObject(const StringView &str, EncodingObject *encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        m_string = str.clone();
    }

    const String &string() const { return m_string; }

    static Value size_fn(Env *env, Value self, Args, Block *) {
        return self->as_string()->size(env);
    }

    static Value bytesize_fn(Env *env, Value self, Args, Block *) {
        auto bytesize = self->as_string()->bytesize();
        return Value::integer(bytesize);
    }

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
    void append(unsigned char);
    void append(const char *);
    void append(const char *, size_t);
    void append(int);
    void append(long unsigned int);
    void append(long long int);
    void append(long int);
    void append(unsigned int);
    void append(double);
    void append(const FloatObject *);
    void append(const IntegerObject *);
    void append(const String &);
    void append(const StringView &);
    void append(const StringObject *);
    void append(const SymbolObject *);
    void append(Value);

    void append_sprintf(const char *format, ...) {
        va_list args, args_copy;
        va_start(args, format);
        m_string.append_vsprintf(format, args);
        va_end(args);
    }

    std::pair<bool, StringView> prev_char_result(size_t *) const;
    StringView prev_char(size_t *) const;
    StringView prev_char(Env *, size_t *) const;
    std::pair<bool, StringView> next_char_result(size_t *) const;
    StringView next_char(size_t *) const;
    StringView next_char(Env *, size_t *) const;

    Value each_char(Env *, Block *);
    Value chars(Env *, Block * = nullptr);

    Value each_codepoint(Env *, Block *);
    Value codepoints(Env *, Block *);

    Value each_line(Env *, Value = nullptr, Value = nullptr, Block * = nullptr);
    Value lines(Env *, Value = nullptr, Value = nullptr, Block * = nullptr);

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

    bool operator!=(const StringObject &value) const {
        return m_string != value.m_string;
    }

    bool operator==(const String &str) const {
        return str == m_string;
    }

    bool operator!=(const String &str) const {
        return str != m_string;
    }

    bool operator==(const StringView &view) const {
        return view == m_string;
    }

    bool operator!=(const StringView &view) const {
        return view != m_string;
    }

    bool operator==(const char *str) const {
        return m_string == str;
    }

    bool operator!=(const char *str) const {
        return m_string != str;
    }

    StringObject *successive(Env *);
    StringObject *successive_in_place(Env *);

    Value index(Env *, Value, Value) const;
    Value index(Env *, Value, size_t start) const;
    nat_int_t index_int(Env *, Value, size_t start) const;

    void truncate(size_t length) {
        m_string.truncate(length);
    }

    Value initialize(Env *, Value, Value, Value);
    Value initialize_copy(Env *, Value);
    Value ltlt(Env *, Value);

    bool eql(Value arg) const {
        return *this == *arg;
    }

    Value to_s() {
        return this;
    }

    bool internal_start_with(Env *, Value);
    bool start_with(Env *, Args);
    bool end_with(Env *, Value) const;
    bool end_with(Env *, Args) const;
    bool is_empty() const { return m_string.is_empty(); }

    Value gsub(Env *, Value, Value = nullptr, Block *block = nullptr);
    Value gsub_in_place(Env *, Value, Value = nullptr, Block *block = nullptr);
    Value getbyte(Env *, Value) const;
    Value sub(Env *, Value, Value = nullptr, Block *block = nullptr);
    Value sub_in_place(Env *, Value, Value = nullptr, Block *block = nullptr);

    Value add(Env *, Value) const;
    bool ascii_only(Env *) const;
    Value b(Env *) const;
    Value bytes(Env *, Block *);
    Value byteslice(Env *, Value, Value);
    StringObject *capitalize(Env *, Value, Value);
    Value capitalize_in_place(Env *, Value, Value);
    Value casecmp(Env *, Value);
    Value is_casecmp(Env *, Value);
    Value center(Env *, Value, Value);
    Value chr(Env *);
    Value chomp(Env *, Value) const;
    Value chomp_in_place(Env *, Value);
    Value chop(Env *) const;
    Value chop_in_place(Env *);
    Value clear(Env *);
    Value cmp(Env *, Value);
    Value concat(Env *env, Args args);
    Value count(Env *env, Args args);
    Value crypt(Env *, Value);
    Value delete_prefix(Env *, Value);
    Value delete_prefix_in_place(Env *, Value);
    Value delete_suffix(Env *, Value);
    Value delete_suffix_in_place(Env *, Value);
    StringObject *downcase(Env *, Value, Value);
    Value downcase_in_place(Env *, Value, Value);
    Value each_byte(Env *, Block *);
    Value encode(Env *, Value);
    bool eq(Env *, Value arg);
    Value eqtilde(Env *, Value);
    Value force_encoding(Env *, Value);
    bool has_match(Env *, Value, Value = nullptr);
    Value hex(Env *) const;
    bool include(Env *, Value);
    bool include(const char *) const;
    Value insert(Env *, Value, Value);
    Value ljust(Env *, Value, Value) const;
    Value lstrip(Env *) const;
    Value lstrip_in_place(Env *);
    Value match(Env *, Value, Value = nullptr, Block * = nullptr);
    Value mul(Env *, Value) const;
    Value ord(Env *) const;
    Value partition(Env *, Value);
    Value prepend(Env *, Args);
    Value ref(Env *, Value, Value = nullptr);
    Value ref_slice_range_in_place(size_t, size_t);
    Value ref_fast_index(Env *, size_t) const;
    Value ref_fast_range(Env *, size_t, size_t) const;
    Value ref_fast_range_endless(Env *, size_t) const;
    Value refeq(Env *, Value, Value, Value);
    Value reverse(Env *);
    Value reverse_in_place(Env *);
    Value rjust(Env *, Value, Value) const;
    Value rstrip(Env *) const;
    Value rstrip_in_place(Env *);
    size_t char_count(Env *) const;
    Value scan(Env *, Value, Block * = nullptr);
    Value setbyte(Env *, Value, Value);
    Value size(Env *) const;
    Value slice_in_place(Env *, Value, Value);
    Value split(Env *, Value, Value);
    Value split(Env *, RegexpObject *, int);
    Value split(Env *, StringObject *, int);
    Value strip(Env *) const;
    Value strip_in_place(Env *);
    Value sum(Env *, Value = nullptr);
    StringObject *swapcase(Env *, Value, Value);
    Value swapcase_in_place(Env *, Value, Value);
    Value to_f(Env *) const;
    Value to_i(Env *, Value = nullptr) const;
    Value tr(Env *, Value, Value) const;
    Value tr_in_place(Env *, Value, Value);
    static Value try_convert(Env *, Value);
    Value uminus(Env *);
    Value unpack(Env *, Value, Value = nullptr) const;
    Value unpack1(Env *, Value, Value = nullptr) const;
    StringObject *upcase(Env *, Value, Value);
    Value upcase_in_place(Env *, Value, Value);
    Value uplus(Env *);
    Value upto(Env *, Value, Value = nullptr, Block * = nullptr);

    Value convert_float();
    Value convert_integer(Env *, nat_int_t base);

    static size_t byte_index_to_char_index(ArrayObject *chars, size_t byte_index);

    size_t char_index_to_byte_index(size_t) const;
    size_t byte_index_to_char_index(size_t) const;

    static CaseFoldType check_case_options(Env *env, Value arg1, Value arg2, CaseFoldType flags);

    unsigned char at(size_t index) const {
        return m_string.at(index);
    }

    template <typename... Args>
    static StringObject *format(const char *fmt, Args... args) {
        auto out = new StringObject;
        format(out, fmt, args...);
        return out;
    }

    static void format(StringObject *out, const char *fmt) {
        for (const char *c = fmt; *c != 0; c++) {
            out->append_char(*c);
        }
    }

    template <typename T, typename... Args>
    static void format(StringObject *out, const char *fmt, T first, Args... rest) {
        for (const char *c = fmt; *c != 0; c++) {
            if (*c == '{' && *(c + 1) == '}') {
                out->append(first);
                format(out, c + 2, rest...);
                return;
            } else if (*c == '{' && *(c + 1) == 'v' && *(c + 2) == '}') {
                if constexpr (std::is_same_v<Value, std::remove_const<T>> || std::is_base_of_v<Object, std::remove_pointer_t<std::remove_const_t<T>>>)
                    out->append(first->dbg_inspect());
                else
                    out->append(first);
                format(out, c + 3, rest...);
                return;
            } else {
                out->append_char(*c);
            }
        }
    }

    virtual String dbg_inspect() const override;

    virtual void visit_children(Visitor &visitor) override final {
        Object::visit_children(visitor);
        visitor.visit(m_encoding);
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
        // cannot be heap-allocated, because the GC is not aware of it.
        void *operator new(size_t size) = delete;

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
    void regexp_sub(Env *, TM::String &, StringObject *, RegexpObject *, StringObject *, MatchDataObject **, StringObject **, size_t = 0, Block *block = nullptr);
    nat_int_t unpack_offset(Env *, Value) const;

    using Object::Object;

    String m_string {};
    EncodingObject *m_encoding { nullptr };
};

}
