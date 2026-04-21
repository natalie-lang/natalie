#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Utf16LeEncodingObject : public EncodingObject {
public:
    Utf16LeEncodingObject()
        : EncodingObject { Encoding::UTF_16LE, { "UTF-16LE" } } { }

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

    virtual int code_unit_size() const override { return 2; }

    virtual int expected_byte_count(const String &string, size_t index) const override {
        if (index + 1 >= string.size()) return 2;
        size_t code_unit = (unsigned char)string[index] + ((unsigned char)string[index + 1] << 8);
        if (code_unit >= 0xD800 && code_unit <= 0xDBFF) return 4;
        return 2;
    }

    virtual bool is_char_boundary(const String &string, size_t byte_offset) const override {
        if (byte_offset == 0 || byte_offset >= string.size()) return true;
        if (byte_offset % 2 != 0 || byte_offset + 2 > string.size()) return false;
        return ((unsigned char)string[byte_offset + 1] & 0xFC) != 0xDC;
    }
};
}
