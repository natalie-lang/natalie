#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Ascii8BitEncodingObject : public EncodingObject {
public:
    Ascii8BitEncodingObject()
        : EncodingObject { Encoding::ASCII_8BIT, { "ASCII-8BIT", "BINARY" } } { }

    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        return codepoint >= 0 && codepoint <= 255;
    }

    virtual StringView next_char(const String &string, size_t *index) const override;

    virtual String escaped_char(unsigned char c) const override;

    virtual Value encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const override;

    virtual String encode_codepoint(nat_int_t codepoint) const override;
};

}
