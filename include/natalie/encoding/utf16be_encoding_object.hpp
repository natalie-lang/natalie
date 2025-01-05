#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Utf16BeEncodingObject : public EncodingObject {
public:
    Utf16BeEncodingObject()
        : EncodingObject { Encoding::UTF_16BE, { "UTF-16BE" } } { }

    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        // 0x0..0x10FFFF are valid, with exception of 0xD800-0xDFFF
        return (codepoint >= 0 && codepoint < 0xD800) || (codepoint > 0xDFFF && codepoint <= 0x10FFFF);
    }
    virtual bool in_encoding_codepoint_range(nat_int_t codepoint) const override {
        // it's positive and takes 1-4 bytes
        return codepoint >= 0 && codepoint < 0x10000000000;
    }

    virtual std::pair<bool, StringView> prev_char(const String &string, size_t *index) const override;
    virtual std::pair<bool, StringView> next_char(const String &string, size_t *index) const override;

    virtual void append_escaped_char(String &str, nat_int_t c) const override;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const override;

    virtual String encode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t decode_codepoint(StringView &str) const override;

    virtual bool is_single_byte_encoding() const override final { return false; }
};

}
