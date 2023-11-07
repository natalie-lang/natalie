#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/array_object.hpp"
#include "natalie/class_object.hpp"
#include "natalie/encodings.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "tm/string_view.hpp"

namespace Natalie {

extern nat_int_t lcase_map[];
extern nat_int_t ucase_map[];
extern nat_int_t tcase_map[];
extern nat_int_t lcase_index[];
extern nat_int_t ucase_index[];
extern nat_int_t tcase_index[];

using namespace TM;

class EncodingObject : public Object {
public:
    EncodingObject();

    EncodingObject(ClassObject *klass)
        : Object { Object::Type::Encoding, klass } { }

    EncodingObject(Encoding, std::initializer_list<const String>);

    // Try to get rid of this
    Encoding num() const { return m_num; }

    const StringObject *name() const;
    Value name(Env *);

    ArrayObject *names(Env *) const;

    Value inspect(Env *) const;

    virtual bool in_encoding_codepoint_range(nat_int_t codepoint) { NAT_UNREACHABLE(); }
    virtual bool is_ascii_compatible() const { return false; } // default
    virtual bool is_dummy() const { return false; }

    virtual bool valid_codepoint(nat_int_t codepoint) const = 0;
    virtual std::pair<bool, StringView> prev_char(const String &, size_t *) const = 0;
    virtual std::pair<bool, StringView> next_char(const String &, size_t *) const = 0;
    virtual String escaped_char(unsigned char c) const = 0;
    virtual Value encode(Env *, EncodingObject *, StringObject *) const;
    virtual String encode_codepoint(nat_int_t codepoint) const = 0;
    virtual nat_int_t decode_codepoint(StringView &str) const = 0;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const = 0;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const = 0;

    virtual bool is_single_byte_encoding() const = 0;

    [[noreturn]] void raise_encoding_invalid_byte_sequence_error(Env *env, const String &, size_t) const;

    static HashObject *aliases(Env *);
    static Value find(Env *, Value);
    static ArrayObject *list(Env *env);
    static ArrayObject *name_list(Env *env);
    static const TM::Hashmap<Encoding, EncodingObject *> &encodings() { return EncodingObject::s_encoding_list; }
    static EncodingObject *default_external() { return s_default_external; }
    static EncodingObject *default_internal() { return s_default_internal; }
    static EncodingObject *locale() { return s_locale; }
    static EncodingObject *filesystem() { return s_filesystem; }
    static EncodingObject *set_default_external(Env *, Value);
    static EncodingObject *set_default_internal(Env *, Value);
    static EncodingObject *get(Encoding encoding) { return s_encoding_list.get(encoding); }
    static Value locale_charmap();
    static void initialize_defaults(Env *);

    static EncodingObject *find_encoding_by_name(Env *env, String name);
    static EncodingObject *find_encoding(Env *env, Value encoding);

    static nat_int_t codepoint_to_lowercase(nat_int_t codepoint);
    static nat_int_t codepoint_to_uppercase(nat_int_t codepoint);
    static nat_int_t codepoint_to_titlecase(nat_int_t codepoint);

    static Value casefold_common(nat_int_t codepoint);
    static Value casefold_full(nat_int_t codepoint);
    static Value casefold_simple(nat_int_t codepoint);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<EncodingObject %p>", this);
    }

private:
    Vector<String> m_names {};
    Encoding m_num;

    static inline TM::Hashmap<Encoding, EncodingObject *> s_encoding_list {};
    static inline EncodingObject *s_default_internal = nullptr;
    // external, locale and filesystem are set by a static initializer function
    static inline EncodingObject *s_default_external = nullptr;
    static inline EncodingObject *s_locale = nullptr;
    static inline EncodingObject *s_filesystem = nullptr;
};

}
