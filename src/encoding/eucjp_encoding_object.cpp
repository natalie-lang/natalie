#include "natalie/encoding/eucjp_encoding_object.hpp"
#include "natalie.hpp"

namespace Natalie {

bool EucJpEncodingObject::valid_codepoint(nat_int_t codepoint) const {
    if (codepoint < 0 || codepoint > 0x8ffefe)
        return false;
    if (codepoint <= 0x7F)
        return true;

    auto b1 = (codepoint & 0x0000FF);
    auto b2 = (codepoint & 0x00FF00) >> 8;
    auto b3 = (codepoint & 0xFF0000) >> 16;

    if (b3 == 0 && b1 >= 0xA1 && b1 <= 0xFE && b1 != 0x8E) {
        if (b2 >= 0xD0 && b2 <= 0xFE)
            return true;
        if ((b2 >= 0xA1 && b2 <= 0xCF) || b2 == 0x8E)
            return true;
        return false;
    }

    if (b3 >= 0xA1 && b3 <= 0xFE && b2 >= 0xA1 && b2 <= 0xFE)
        return true;

    if (b3 == 0x8F && b2 >= 0xA1 && b2 <= 0xFE && b1 >= 0xA1 && b1 <= 0xFE)
        return true;

    return false;
}

std::pair<bool, StringView> EucJpEncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    size_t length = 1;
    (*index)--;
    unsigned char c = string[*index];

    if ((int)c <= 0x7F) {
        // 7F or below is a single-byte
        return { true, StringView(&string, *index, 1) };
    }
    if (((int)c >= 0x80 && (int)c <= 0xA0) || (int)c == 0xFF) {
        // invalid code
        return { false, StringView(&string, *index, 1) };
    }
    NAT_NOT_YET_IMPLEMENTED();
}

std::pair<bool, StringView> EucJpEncodingObject::next_char(const String &string, size_t *index) const {
    size_t len = string.size();
    if (*index >= len)
        return { true, StringView() };
    size_t i = *index;
    int length = 0;
    unsigned char c = string[i];
    if (c < 0x80) {
        // is a single byte char, no trail byte.
        *index += 1;
        return { true, StringView(&string, i, 1) };
    } else if (c == 0x8E || (c >= 0xA1 && c <= 0xFE)) {
        length = 2;
    } else if (c == 0x8F) {
        length = 3;
    } else {
        *index += 1;
        return { false, StringView(&string, i, 1) };
    }
    if (i + length > len) {
        // missing required remaining bytes, this is bad.
        *index = len;
        return { false, StringView(&string, i) };
    }
    // validate any mid/trail bytes.
    for (size_t j = i + 1; j <= i + length - 1; j++) {
        unsigned char cj = string[j];
        if (!(cj >= 0xA1 && cj <= 0xFE)) {
            *index += 1;
            return { false, StringView(&string, i, 1) };
        }
    }
    *index += length;
    return { true, StringView(&string, i, length) };
}

String EucJpEncodingObject::escaped_char(unsigned char c) const {
    long long cint = (long long)c;
    char buf[7];
    if (cint <= 0xFF) {
        snprintf(buf, 5, "\\x%02llX", cint);
    } else {
        snprintf(buf, 7, "\\u%04llX", cint);
    }
    return String(buf);
}

nat_int_t EucJpEncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;
    NAT_NOT_YET_IMPLEMENTED("Conversion above Unicode Basic Latin (0x00..0x7F) not implemented");
}

nat_int_t EucJpEncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;
    NAT_NOT_YET_IMPLEMENTED("Conversion above Unicode Basic Latin (0x00..0x7F) not implemented");
}

String EucJpEncodingObject::encode_codepoint(nat_int_t codepoint) const {
    String buf;
    if (codepoint > 0xffff) buf.append_char(codepoint >> 16);
    if (codepoint > 0xff) buf.append_char(codepoint >> 8 & 0xff);
    buf.append_char(codepoint & 0xff);
    return buf;
}

nat_int_t EucJpEncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    case 2:
        return ((unsigned char)str[0] << 8)
            + ((unsigned char)str[1]);
    case 3:
        return ((unsigned char)str[0] << 16)
            + ((unsigned char)str[1] << 8)
            + ((unsigned char)str[2]);
    default:
        return -1;
    }
}

}
