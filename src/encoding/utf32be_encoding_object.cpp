#include "natalie.hpp"
#include <stdio.h>

namespace Natalie {

std::pair<bool, StringView> Utf32BeEncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };

    if (*index < 4) {
        size_t length = *index;
        *index = 0;
        return { false, StringView(&string, *index, length) };
    }

    *index -= 4;
    StringView character = StringView(&string, *index, 4);

    // check codepoint
    nat_int_t codepoint = decode_codepoint(character);
    if (codepoint < 0 || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return { false, character };
    }

    return { true, character };
}

std::pair<bool, StringView> Utf32BeEncodingObject::next_char(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, StringView() };

    if (*index + 4 > string.size()) {
        size_t length = string.size() - *index;
        auto i = *index;
        (*index) = string.size();
        return { false, StringView(&string, i, length) };
    }

    StringView character = StringView(&string, *index, 4);
    (*index) += 4;

    // check codepoint
    nat_int_t codepoint = decode_codepoint(character);
    if (codepoint < 0 || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return { false, character };
    }

    return { true, character };
}

String Utf32BeEncodingObject::escaped_char(unsigned char c) const {
    char buf[7];
    snprintf(buf, 7, "\\u%04llX", (long long)c);
    return String(buf);
}

nat_int_t Utf32BeEncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

nat_int_t Utf32BeEncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

String Utf32BeEncodingObject::encode_codepoint(nat_int_t codepoint) const {
    String buf;

    buf.append_char(0);
    buf.append_char((codepoint >> 16) & 0xFF);
    buf.append_char((codepoint >> 8) & 0xFF);
    buf.append_char(codepoint & 0xFF);

    return buf;
}

nat_int_t Utf32BeEncodingObject::decode_codepoint(StringView &str) const {
    if (str.size() != 4) {
        return -1;
    }

    return ((unsigned char)str[0] << 24)
        + ((unsigned char)str[1] << 16)
        + ((unsigned char)str[2] << 8)
        + (unsigned char)str[3];
}

}
