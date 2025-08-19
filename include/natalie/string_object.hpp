#pragma once

#include <assert.h>
#include <functional>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "natalie/class_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "tm/non_null_ptr.hpp"

namespace Natalie {

using namespace TM;

class StringObject : public Object {
public:
    enum class Chilled {
        None,
        String,
        Symbol,
    };

    enum class Validity {
        Unknown,
        Valid,
        Invalid,
    };

    template <typename... Args>
    static StringObject *create(Args &&...args) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new StringObject(std::forward<Args>(args)...);
    }

    const String &string() const { return m_string; }

    static Value size_fn(Env *env, Value self, Args &&, Block *) {
        return self.as_string()->size(env);
    }

    static Value bytesize_fn(Env *env, Value self, Args &&, Block *) {
        auto bytesize = self.as_string()->bytesize();
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

    void set_str(String &&str) {
        m_string = std::move(str);
    }

    bool is_chilled() const { return m_chilled != Chilled::None; }
    Chilled chilled() const { return m_chilled; }
    void set_chilled(Chilled chilled) { m_chilled = chilled; }
    void unset_chilled() { m_chilled = Chilled::None; }

    bool valid_encoding() const {
        if (m_validity != Validity::Unknown)
            return m_validity == Validity::Valid;
        auto valid = check_valid_encoding();
        m_validity = valid ? Validity::Valid : Validity::Invalid;
        return valid;
    }

    void update_validity(StringObject *other) {
        if (m_validity == Validity::Valid)
            m_validity = other->valid_encoding() ? Validity::Valid : Validity::Invalid;
        else
            // NOTE: Here we assume that it's *possible* for two invalid strings
            // to be combined in a way that produces a valid string.
            // I'm not sure if that's really possible though!
            m_validity = Validity::Unknown;
    }

    void force_validity(Validity validity) {
        m_validity = validity;
    }

    EncodingObject *encoding() const { return m_encoding.ptr(); }
    void set_encoding(EncodingObject *encoding) { m_encoding = encoding; }
    bool is_ascii_only() const;
    EncodingObject *negotiate_compatible_encoding(const StringObject *) const;
    void assert_compatible_string(Env *, const StringObject *) const;
    void assert_valid_encoding(Env *) const;
    EncodingObject *assert_compatible_string_and_update_encoding(Env *, StringObject *);

    void prepend_char(Env *, char);

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
    std::pair<bool, StringView> peek_char_result(size_t) const;
    std::pair<bool, StringView> next_char_result(size_t *) const;
    StringView peek_char(size_t) const;
    StringView next_char(size_t *) const;
    StringView next_char(Env *, size_t *) const;

    Value each_char(Env *, Block *);
    Value chars(Env *, Block * = nullptr);

    Value each_codepoint(Env *, Block *);
    Value codepoints(Env *, Block *);

    Value each_grapheme_cluster(Env *, Block *);
    Value grapheme_clusters(Env *, Block *);

    void each_line(Env *, Optional<Value>, Optional<Value>, std::function<Value(StringObject *)>) const;
    Value each_line(Env *, Optional<Value> = {}, Optional<Value> = {}, Block * = nullptr);
    Value lines(Env *, Optional<Value> = {}, Optional<Value> = {}, Block * = nullptr);

    SymbolObject *to_symbol(Env *) const;
    Value to_sym(Env *) const;

    StringObject *inspect(Env *) const;

    StringObject &operator=(StringObject other) {
        this->m_string = other.m_string;
        this->m_encoding = other.m_encoding;
        return *this;
    }

    bool operator==(Value value) const {
        if (!value.is_string())
            return false;
        return *this == *(value.as_string());
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

    Value byteindex(Env *, Value, Optional<Value> = {}) const;
    Value byterindex(Env *, Value, Optional<Value> = {}) const;

    Value index(Env *, Value, Optional<Value>) const;
    Value index(Env *, Value, size_t start) const;
    nat_int_t index_int(Env *, Value, size_t byte_start) const;

    Value rindex(Env *, Value, Optional<Value> = {}) const;
    Value rindex(Env *, Value, size_t) const;
    nat_int_t rindex_int(Env *, Value, size_t) const;

    void truncate(size_t length) {
        m_string.truncate(length);
    }

    Value initialize(Env *, Optional<Value>, Optional<Value>, Optional<Value>);
    Value initialize_copy(Env *, Value);
    Value ltlt(Env *, Value);

    bool eql(Value arg) const {
        return *this == arg;
    }

    StringObject *to_s();

    bool internal_start_with(Env *, Value);
    bool start_with(Env *, Args &&);
    bool end_with(Env *, Value) const;
    bool end_with(Env *, Args &&) const;
    bool is_empty() const { return m_string.is_empty(); }

    Value gsub(Env *, Value, Optional<Value> = {}, Block *block = nullptr);
    Value gsub_in_place(Env *, Value, Optional<Value> = {}, Block *block = nullptr);
    Value getbyte(Env *, Value) const;
    Value sub(Env *, Value, Optional<Value> = {}, Block *block = nullptr);
    Value sub_in_place(Env *, Value, Optional<Value> = {}, Block *block = nullptr);

    Value add(Env *, Value) const;
    Value append_as_bytes(Env *, Args &&);
    Value b(Env *) const;
    Value bytes(Env *, Block *);
    Value byteslice(Env *, Value, Optional<Value> = {});
    Value bytesplice(Env *, Args &&);
    StringObject *capitalize(Env *, Optional<Value>, Optional<Value>);
    Value capitalize_in_place(Env *, Optional<Value>, Optional<Value>);
    Value casecmp(Env *, Value);
    Value is_casecmp(Env *, Value);
    Value center(Env *, Value, Optional<Value>);
    Value chr(Env *);
    Value chomp(Env *, Optional<Value>) const;
    Value chomp_in_place(Env *, Optional<Value> = {});
    Value chop(Env *) const;
    Value chop_in_place(Env *);
    Value clear(Env *);
    Value cmp(Env *, Value);
    Value concat(Env *env, Args &&args);
    Value count(Env *env, Args &&args);
    Value crypt(Env *, Value);
    Value delete_str(Env *, Args &&);
    Value delete_in_place(Env *, Args &&);
    Value delete_prefix(Env *, Value);
    Value delete_prefix_in_place(Env *, Value);
    Value delete_suffix(Env *, Value);
    Value delete_suffix_in_place(Env *, Value);
    StringObject *downcase(Env *, Optional<Value> = {}, Optional<Value> = {});
    Value downcase_in_place(Env *, Optional<Value> = {}, Optional<Value> = {});
    Value dump(Env *);
    Value each_byte(Env *, Block *);
    Value encode(Env *, Optional<Value> = {}, Optional<Value> = {}, HashObject * = nullptr);
    Value encode_in_place(Env *, Optional<Value> = {}, Optional<Value> = {}, HashObject * = nullptr);
    bool eq(Env *, Value arg);
    Value eqtilde(Env *, Value);
    Value force_encoding(Env *, Value);
    bool has_match(Env *, Value, Optional<Value> = {});
    Value hex(Env *) const;
    bool include(Env *, Value);
    bool include(const char *) const;
    bool include(Env *, nat_int_t) const;
    Value insert(Env *, Value, Value);
    Value ljust(Env *, Value, Optional<Value>) const;
    Value lstrip(Env *) const;
    Value lstrip_in_place(Env *);
    Value match(Env *, Value, Optional<Value> = {}, Block * = nullptr);
    Value mul(Env *, Value) const;
    Value ord(Env *) const;
    Value partition(Env *, Value);
    Value prepend(Env *, Args &&);
    Value ref(Env *, Value, Optional<Value> = {});
    Value ref_slice_range_in_place(size_t, size_t);
    Value ref_fast_index(Env *, size_t) const;
    Value ref_fast_range(Env *, size_t, size_t) const;
    Value ref_fast_range_endless(Env *, size_t) const;
    Value refeq(Env *, Value, Optional<Value>, Optional<Value>);
    Value reverse(Env *);
    Value reverse_in_place(Env *);
    Value rjust(Env *, Value, Optional<Value>) const;
    Value rstrip(Env *) const;
    Value rstrip_in_place(Env *);
    size_t char_count(Env *) const;
    Value scan(Env *, Value, Block * = nullptr);
    Value setbyte(Env *, Value, Value);
    Value size(Env *) const;
    Value slice_in_place(Env *, Value, Optional<Value>);
    Value split(Env *, Optional<Value>, Optional<Value> = {});
    Value split(Env *, RegexpObject *, int);
    Value split(Env *, StringObject *, int);
    Value squeeze(Env *env, Args &&selectors);
    Value squeeze_in_place(Env *env, Args &&selectors);
    Value squeeze_in_place_without_selectors(Env *env);
    Value strip(Env *) const;
    Value strip_in_place(Env *);
    Value sum(Env *, Optional<Value> = {});
    StringObject *swapcase(Env *, Optional<Value>, Optional<Value>);
    Value swapcase_in_place(Env *, Optional<Value>, Optional<Value>);
    Value to_c(Env *);
    Value to_f(Env *) const;
    Value to_i(Env *, Optional<Value> = {}) const;
    Value to_r(Env *) const;
    Value tr(Env *, Value, Value) const;
    Value tr_in_place(Env *, Value, Value);
    static Value try_convert(Env *, Value);
    Value uminus(Env *);
    Value undump(Env *) const;
    Value unpack(Env *, Value, Optional<Value> = {}) const;
    Value unpack1(Env *, Value, Optional<Value> = {}) const;
    StringObject *upcase(Env *, Optional<Value> = {}, Optional<Value> = {});
    Value upcase_in_place(Env *, Optional<Value> = {}, Optional<Value> = {});
    Value uplus(Env *);
    Value upto(Env *, Value, Optional<Value> = {}, Block * = nullptr);

    Value convert_float();
    Value convert_integer(Env *, nat_int_t base);

    static size_t byte_index_to_char_index(ArrayObject *chars, size_t byte_index);

    ssize_t char_index_to_byte_index(ssize_t, bool = false) const;
    ssize_t byte_index_to_char_index(size_t) const;

    static uint8_t check_case_options(Env *env, Value arg1, Value arg2, bool downcase = false);
    static uint8_t check_case_options(Env *env, Optional<Value> arg1, Optional<Value> arg2, bool downcase = false) {
        return check_case_options(env, arg1.value_or(Value::nil()), arg2.value_or(Value::nil()), downcase);
    }

    unsigned char at(size_t index) const {
        return m_string.at(index);
    }

    template <typename... Args>
    static StringObject *format(const char *fmt, Args... args) {
        auto out = StringObject::create();
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

    virtual String dbg_inspect(int indent = 0) const override;

    virtual void visit_children(Visitor &visitor) const override final {
        Object::visit_children(visitor);
        visitor.visit(m_encoding.ptr());
    }

    virtual bool is_large() override {
        return m_string.capacity() >= 100;
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
    StringObject(ClassObject *klass)
        : Object { Object::Type::String, klass }
        , m_encoding { EncodingObject::get(Encoding::ASCII_8BIT) } {
        assert(m_encoding);
    }

    StringObject()
        : StringObject { "" } { }

    StringObject(NonNullPtr<EncodingObject> encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        set_str("", 0);
    }

    StringObject(const char *str)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(Encoding::UTF_8) } {
        assert(m_encoding);
        set_str(str);
    }

    StringObject(const char *str, NonNullPtr<EncodingObject> encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        set_str(str);
    }

    StringObject(const char *str, Encoding encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(encoding) } {
        assert(m_encoding);
        set_str(str);
    }

    StringObject(const char *str, size_t length)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(Encoding::UTF_8) } {
        assert(m_encoding);
        set_str(str, length);
    }

    StringObject(const char *str, size_t length, NonNullPtr<EncodingObject> encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        set_str(str, length);
    }

    StringObject(const char *str, size_t length, Encoding encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { EncodingObject::get(encoding) } {
        assert(m_encoding);
        set_str(str, length);
    }

    StringObject(const StringObject &other)
        : Object { other }
        , m_encoding { other.m_encoding }
        , m_validity { other.m_validity } {
        set_str(other.c_str(), other.length());
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

    StringObject(const String &str, NonNullPtr<EncodingObject> encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        m_string = str;
    }

    StringObject(String &&str, NonNullPtr<EncodingObject> encoding)
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

    StringObject(const StringView &str, NonNullPtr<EncodingObject> encoding)
        : Object { Object::Type::String, GlobalEnv::the()->String() }
        , m_encoding { encoding } {
        assert(m_encoding);
        m_string = str.clone();
    }

    StringObject *expand_backrefs(Env *, StringObject *, MatchDataObject *);
    void regexp_sub(Env *, TM::String &, StringObject *, RegexpObject *, Optional<Value>, MatchDataObject **, StringObject **, size_t = 0, Block *block = nullptr);
    nat_int_t unpack_offset(Env *, Optional<Value>) const;

    bool check_valid_encoding() const {
        return m_encoding->check_string_valid_in_encoding(m_string);
    }

    using Object::Object;

    using EncodeOptions = EncodingObject::EncodeOptions;
    using EncodeInvalidOption = EncodingObject::EncodeInvalidOption;
    using EncodeNewlineOption = EncodingObject::EncodeNewlineOption;
    using EncodeXmlOption = EncodingObject::EncodeXmlOption;
    using EncodeUndefOption = EncodingObject::EncodeUndefOption;

    String m_string {};
    NonNullPtr<EncodingObject> m_encoding;
    Chilled m_chilled { Chilled::None };
    mutable Validity m_validity { Validity::Unknown };
};

}
