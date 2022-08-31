#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Utf8EncodingObject : public EncodingObject {
public:
    Utf8EncodingObject()
        : EncodingObject { Encoding::UTF_8, { "UTF-8" } } { }

    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        // from RFC3629: 0x0..0x10FFFF are valid, with exception of 0xD800-0xDFFF
        return (codepoint >= 0 && codepoint < 0xD800) || (codepoint > 0xDFFF && codepoint <= 0x10FFFF);
    }

    virtual std::pair<bool, StringView> prev_char(const String &string, size_t *index) const override;
    virtual std::pair<bool, StringView> next_char(const String &string, size_t *index) const override;

    virtual String escaped_char(unsigned char c) const override;

    virtual Value encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const override;

    virtual String encode_codepoint(nat_int_t codepoint) const override;
};

}
