#include "natalie/encoding/big5_encoding_object.hpp"
#include "natalie.hpp"

namespace Natalie {

static inline bool is_big5_lead(unsigned char c) {
    return c >= 0xA1 && c <= 0xFE;
}

static inline bool is_big5_trail(unsigned char c) {
    return (c >= 0x40 && c <= 0x7E) || (c >= 0xA1 && c <= 0xFE);
}

bool Big5EncodingObject::valid_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0 && codepoint <= 0x7F)
        return true;
    auto b1 = (codepoint & 0xFF00) >> 8;
    auto b2 = (codepoint & 0xFF);
    return is_big5_lead(b1) && is_big5_trail(b2);
}

std::pair<bool, StringView> Big5EncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };

    (*index)--;

    unsigned char c = string[*index];

    if (is_big5_trail(c) && *index > 0) {
        unsigned char c_prev = string[*index - 1];
        if (is_big5_lead(c_prev)) {
            (*index)--;
            return { true, StringView(&string, *index, 2) };
        }
    }

    if (c <= 0x7F)
        return { true, StringView(&string, *index, 1) };

    return { false, StringView(&string, *index, 1) };
}

std::pair<bool, StringView> Big5EncodingObject::next_char(const String &string, size_t *index) const {
    size_t len = string.size();
    if (*index >= len)
        return { true, StringView() };
    size_t i = *index;
    unsigned char c = string[i];

    if (is_big5_lead(c)) {
        if (i + 1 >= len) {
            *index = len;
            return { false, StringView(&string, i) };
        }
        unsigned char c2 = string[i + 1];
        if (is_big5_trail(c2)) {
            *index += 2;
            return { true, StringView(&string, i, 2) };
        }
        *index += 1;
        return { false, StringView(&string, i, 1) };
    }

    *index += 1;
    bool valid = c <= 0x7F;
    return { valid, StringView(&string, i, 1) };
}

void Big5EncodingObject::append_escaped_char(String &str, nat_int_t c) const {
    if (c >= 0 && c <= 0xFF) {
        str.append_sprintf("\\x%02llX", c);
    } else {
        str.append_sprintf("\\x{%04llX}", c);
    }
}

nat_int_t Big5EncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0 && codepoint <= 0x7F)
        return codepoint;
    return -1;
}

nat_int_t Big5EncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0 && codepoint <= 0x7F)
        return codepoint;
    return -1;
}

String Big5EncodingObject::encode_codepoint(nat_int_t codepoint) const {
    String buf;
    if (codepoint > 0xFF) {
        buf.append_char((codepoint & 0xFF00) >> 8);
        buf.append_char(codepoint & 0xFF);
    } else {
        buf.append_char(codepoint);
    }
    return buf;
}

nat_int_t Big5EncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    case 2:
        return ((unsigned char)str[0] << 8) + (unsigned char)str[1];
    default:
        return -1;
    }
}

}
