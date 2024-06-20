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

enum CaseMapType {
    CaseMapFull = 0,
    CaseMapAscii = 1,
    CaseMapTurkicAzeri = 2,
    CaseMapLithuanian = 4,
    CaseMapFold = 8,
};

const int SPECIAL_CASE_LOWER_MAX_SIZE = 2;
const int SPECIAL_CASE_TITLE_MAX_SIZE = 3;
const int SPECIAL_CASE_UPPER_MAX_SIZE = 3;

struct SpecialCasingEntry {
    uint32_t code;
    uint32_t lower[SPECIAL_CASE_LOWER_MAX_SIZE];
    uint32_t title[SPECIAL_CASE_TITLE_MAX_SIZE];
    uint32_t upper[SPECIAL_CASE_UPPER_MAX_SIZE];
};

extern nat_int_t lcase_map[];
extern nat_int_t ucase_map[];
extern nat_int_t tcase_map[];
extern nat_int_t lcase_index[];
extern nat_int_t ucase_index[];
extern nat_int_t tcase_index[];
extern const int special_casing_map_size;
extern SpecialCasingEntry special_casing_map[];

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

    virtual bool in_encoding_codepoint_range(nat_int_t codepoint) const { NAT_UNREACHABLE(); }
    virtual bool is_ascii_compatible() const { return false; } // default
    virtual bool is_dummy() const { return false; }

    virtual bool valid_codepoint(nat_int_t codepoint) const = 0;

    virtual std::pair<bool, StringView> prev_char(const String &, size_t *) const = 0;
    virtual std::pair<bool, StringView> next_char(const String &, size_t *) const = 0;

    virtual StringView next_grapheme_cluster(const String &str, size_t *index) const {
        auto [_valid, view] = next_char(str, index);
        return view;
    }

    enum class EncodeNewlineOption {
        None,
        Cr,
        Crlf,
        Universal,
    };
    virtual Value encode(Env *, EncodingObject *, StringObject *, EncodeNewlineOption = EncodeNewlineOption::None) const;

    virtual bool is_printable_char(const nat_int_t c) const;
    virtual String escaped_char(const nat_int_t c) const = 0;
    virtual String encode_codepoint(nat_int_t codepoint) const = 0;
    virtual nat_int_t decode_codepoint(StringView &str) const = 0;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const = 0;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const = 0;

    virtual bool is_single_byte_encoding() const = 0;

    virtual bool is_compatible_with(EncodingObject *) const;

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

    // must pass a buffer of nat_int_t to this function; uint8_t return is number of codepoints written
    static uint8_t codepoint_to_lowercase(nat_int_t codepoint, nat_int_t result[], CaseMapType flags = CaseMapFull);
    static uint8_t codepoint_to_uppercase(nat_int_t codepoint, nat_int_t result[], CaseMapType flags = CaseMapFull);
    static uint8_t codepoint_to_titlecase(nat_int_t codepoint, nat_int_t result[], CaseMapType flags = CaseMapFull);

    static void init_special_casing_map();
    static SpecialCasingEntry find_special_casing_map_entry(nat_int_t codepoint);

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
