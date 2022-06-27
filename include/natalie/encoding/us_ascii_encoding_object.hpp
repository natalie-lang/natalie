#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class UsAsciiEncodingObject : public EncodingObject {
public:
    UsAsciiEncodingObject()
        : EncodingObject { Encoding::US_ASCII, { "US-ASCII" } } { }

    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        return codepoint >= 0 && codepoint <= 127;
    }

    virtual String next_char(Env *env, String &string, size_t *index) const override;

    virtual String escaped_char(unsigned char c) const override;

    virtual Value encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const override;

    virtual String encode_codepoint(nat_int_t codepoint) const override;
};

}
