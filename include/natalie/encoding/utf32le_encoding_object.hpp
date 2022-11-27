#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Utf32LeEncodingObject : public EncodingObject {
public:
    Utf32LeEncodingObject()
        : EncodingObject { Encoding::UTF_32LE, { "UTF-32LE", "UCS-4LE" } } { }

    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        // 0x0..0x10FFFF are valid, with exception of 0xD800-0xDFFF
        return (codepoint >= 0 && codepoint < 0xD800) || (codepoint > 0xDFFF && codepoint <= 0x10FFFF);
    }

    virtual std::pair<bool, StringView> prev_char(const String &string, size_t *index) const override;
    virtual std::pair<bool, StringView> next_char(const String &string, size_t *index) const override;

    virtual String escaped_char(unsigned char c) const override;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const override;

    virtual String encode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t decode_codepoint(StringView &str) const override;
};

}
