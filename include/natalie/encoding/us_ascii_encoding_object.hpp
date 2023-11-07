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
        : EncodingObject { Encoding::US_ASCII, { "US-ASCII", "ASCII", "ANSI_X3.4-1968", "646" } } { }

    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        return codepoint >= 0 && codepoint <= 127;
    }
    virtual bool in_encoding_codepoint_range(nat_int_t codepoint) override {
        return codepoint >= 0 && codepoint < 128;
    }

    virtual std::pair<bool, StringView> prev_char(const String &string, size_t *index) const override;
    virtual std::pair<bool, StringView> next_char(const String &string, size_t *index) const override;

    virtual String escaped_char(unsigned char c) const override;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const override;

    virtual String encode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t decode_codepoint(StringView &str) const override;
    virtual bool is_ascii_compatible() const override { return true; };

    virtual bool is_single_byte_encoding() const override final { return true; }
};

}
