#pragma once

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class DummyEncodingObject : public EncodingObject {
public:
    DummyEncodingObject(Encoding num, std::initializer_list<const String> names)
        : EncodingObject { num, names } { }

    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        return codepoint >= 0 && codepoint <= 255;
    }
    virtual bool in_encoding_codepoint_range(nat_int_t codepoint) const override {
        return codepoint >= 0 && codepoint < 256;
    }

    virtual bool is_dummy() const override { return true; }
    virtual bool is_single_byte_encoding() const override { return false; }

    virtual std::pair<bool, StringView> prev_char(const String &string, size_t *index) const override;
    virtual std::pair<bool, StringView> next_char(const String &string, size_t *index) const override;

    virtual void append_escaped_char(String &str, nat_int_t c) const override;

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override { return -1; }
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const override { return -1; }

    virtual String encode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t decode_codepoint(StringView &str) const override;

    virtual bool check_string_valid_in_encoding(const String &string) const override { return true; }
};

}
