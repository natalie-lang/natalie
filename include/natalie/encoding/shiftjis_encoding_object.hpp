#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class ShiftJisEncodingObject : public EncodingObject {
public:
    ShiftJisEncodingObject()
        : EncodingObject { Encoding::SHIFT_JIS, { "Shift_JIS", "SJIS" } } { }

    virtual bool valid_codepoint(nat_int_t codepoint) const override;

    virtual bool in_encoding_codepoint_range(nat_int_t codepoint) const override {
        return (codepoint >= 0 && codepoint <= 0xFD39);
    }

    virtual bool is_ascii_compatible() const override { return true; };

    virtual std::pair<bool, StringView> prev_char(const String &string, size_t *index) const override;
    virtual std::pair<bool, StringView> next_char(const String &string, size_t *index) const override;

    virtual void append_escaped_char(String &str, nat_int_t c) const override;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const override;

    virtual String encode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t decode_codepoint(StringView &str) const override;

    virtual bool is_single_byte_encoding() const override final { return false; }

    virtual int expected_byte_count(const String &string, size_t index) const override {
        unsigned char byte = string[index];
        if ((byte >= 0x81 && byte <= 0x9F) || (byte >= 0xE0 && byte <= 0xFC)) return 2;
        return 1;
    }
};

}
