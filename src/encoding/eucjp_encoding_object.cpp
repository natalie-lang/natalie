#include "natalie.hpp"

namespace Natalie {

// NATFIXME: actual implementation is wrong for much of this file
std::pair<bool, StringView> EucJpEncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    (*index)--;
    return { true, StringView(&string, *index, 1) };
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
    char buf[5];
    snprintf(buf, 5, "\\x%02llX", (long long)c);
    return String(buf);
}

nat_int_t EucJpEncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 128)
        return -1;

    return codepoint;
}

nat_int_t EucJpEncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 128)
        return -1;

    return codepoint;
}

String EucJpEncodingObject::encode_codepoint(nat_int_t codepoint) const {
    return String((char)codepoint);
}

nat_int_t EucJpEncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    default:
        return -1;
    }
}

}
