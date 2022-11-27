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

using namespace TM;

class EncodingObject : public Object {
public:
    EncodingObject();

    EncodingObject(ClassObject *klass)
        : Object { Object::Type::Encoding, klass } { }

    EncodingObject(Encoding, std::initializer_list<const String>);

    Encoding num() const { return m_num; }

    const StringObject *name() const;
    Value name(Env *);

    ArrayObject *names(Env *);

    Value inspect(Env *) const;

    bool in_encoding_codepoint_range(nat_int_t codepoint) {
        switch (m_num) {
        case Encoding::ASCII_8BIT:
            return codepoint >= 0 && codepoint < 256;
        case Encoding::US_ASCII:
            return codepoint >= 0 && codepoint < 128;
        case Encoding::UTF_8:
            return codepoint >= 0 && codepoint < 1114112;
        case Encoding::UTF_32LE:
            // it's positive and takes 1-4 bytes
            return codepoint >= 0 && codepoint < 0x10000000000;
        }
        NAT_UNREACHABLE();
    }

    bool is_ascii_compatible() const {
        switch (m_num) {
        case Encoding::ASCII_8BIT:
        case Encoding::US_ASCII:
        case Encoding::UTF_8:
            return true;
        default:
            return false;
        }
    }

    virtual bool valid_codepoint(nat_int_t codepoint) const = 0;
    virtual std::pair<bool, StringView> prev_char(const String &, size_t *) const = 0;
    virtual std::pair<bool, StringView> next_char(const String &, size_t *) const = 0;
    virtual String escaped_char(unsigned char c) const = 0;
    virtual Value encode(Env *, EncodingObject *, StringObject *) const;
    virtual String encode_codepoint(nat_int_t codepoint) const = 0;
    virtual nat_int_t decode_codepoint(StringView &str) const = 0;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const = 0;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const = 0;

    [[noreturn]] void raise_encoding_invalid_byte_sequence_error(Env *env, const String &, size_t) const;

    static HashObject *aliases(Env *);
    static EncodingObject *find(Env *, Value);
    static ArrayObject *list(Env *env);
    static const TM::Hashmap<Encoding, EncodingObject *> &encodings() { return EncodingObject::s_encoding_list; }
    static EncodingObject *default_internal() { return s_default_internal; }
    static EncodingObject *set_default_internal(Env *, Value);
    static EncodingObject *get(Encoding encoding) { return s_encoding_list.get(encoding); }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<EncodingObject %p>", this);
    }

private:
    Vector<String> m_names {};
    Encoding m_num;

    static inline TM::Hashmap<Encoding, EncodingObject *> s_encoding_list {};
    static inline EncodingObject *s_default_internal = nullptr;
};

}
