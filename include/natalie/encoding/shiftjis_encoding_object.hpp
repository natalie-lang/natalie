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
        : EncodingObject { Encoding::SHIFT_JIS, { "Shift_JIS" } } { }

    // https://en.wikipedia.org/wiki/Shift_JIS
    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        if (codepoint < 0) return false;
        if (codepoint > 0xFFFF) return false;
        auto firstbyte = codepoint & 0x00FF;
        if (firstbyte == 0x80 || firstbyte == 0xA0 || firstbyte >= 0xF0)
            return false;
        auto fbhi = firstbyte >> 8;
        bool twobyte = (fbhi == 0x8 || fbhi == 0x9 || fbhi == 0xF);
        if (twobyte) {
            auto secondbyte = codepoint & 0xFF00;
            if (secondbyte < 0x40 || secondbyte == 0x7f || secondbyte >= 0xFD)
                return false;
        }
        return true; // ok single byte
    }
    // NATFIXME : incorrect implementation
    virtual bool in_encoding_codepoint_range(nat_int_t codepoint) override {
        return (codepoint >= 0 && codepoint <= 0xFF);
    }

    virtual bool is_ascii_compatible() const override { return true; };

    virtual std::pair<bool, StringView> prev_char(const String &string, size_t *index) const override;
    virtual std::pair<bool, StringView> next_char(const String &string, size_t *index) const override;

    virtual String escaped_char(unsigned char c) const override;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const override;

    virtual String encode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t decode_codepoint(StringView &str) const override;

    virtual bool is_single_byte_encoding() const override final { return false; }
};

}
