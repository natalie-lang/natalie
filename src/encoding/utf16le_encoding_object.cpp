#include "natalie.hpp"

namespace Natalie {

std::pair<bool, StringView> Utf16LeEncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };

    size_t i = *index;

    // there are not enough bytes left
    if (*index < 2) {
        *index--;
        return { false, StringView(&string, i - 1, 1) };
    }

    size_t code_unit_high = (unsigned char)string[i - 2]
        + ((unsigned char)string[i - 1] << 8);

    //  2-bytes logic

    // single code unit - 0000..D7FF or E000..FFFF
    if (code_unit_high <= 0xD7FF || code_unit_high >= 0xE000) {
        *index -= 2;
        return { true, StringView(&string, i - 2, 2) };
    }

    // there are not enough bytes left
    if (*index < 4) {
        *index--;
        return { false, StringView(&string, i - 1, 1) };
    }

    //  4-bytes logic

    size_t code_unit_low = (unsigned char)string[i - 4]
        + ((unsigned char)string[i - 3] << 8);

    // 1st code unit - D800..DBFF and 2nd - DC00..DFFF
    if (code_unit_low >= 0xD800 && code_unit_low <= 0xDBFF
        && code_unit_high >= 0xDC00 && code_unit_high <= 0xDFFF) {
        *index -= 4;
        return { true, StringView(&string, i - 4, 4) };
    }

    *index -= 2;
    return { false, StringView(&string, i - 2, 2) };
}

/*
    See: https://en.wikipedia.org/wiki/UTF-16
*/
std::pair<bool, StringView> Utf16LeEncodingObject::next_char(const String &string, size_t *index) const {
    size_t len = string.size();
    size_t i = *index;

    if (*index >= len)
        return { true, StringView() };

    // there are not enough bytes left
    if (*index + 1 >= len) {
        *index += 1;
        return { false, StringView(&string, i, 1) };
    }

    size_t code_unit_low = (unsigned char)string[i]
        + ((unsigned char)string[i + 1] << 8);

    //  2-bytes logic

    // single code unit - 0000..D7FF or E000..FFFF
    if (code_unit_low <= 0xD7FF || code_unit_low >= 0xE000) {
        *index += 2;
        return { true, StringView(&string, i, 2) };
    }

    // there are not enough bytes left
    if (*index + 3 >= len) {
        *index += 2;
        return { false, StringView(&string, i, 2) };
    }

    //  4-bytes logic

    size_t code_unit_high = (unsigned char)string[i + 2]
        + ((unsigned char)string[i + 3] << 8);

    // 1st code unit - D800..DBFF and 2nd - DC00..DFFF
    if (code_unit_low >= 0xD800 && code_unit_low <= 0xDBFF
        && code_unit_high >= 0xDC00 && code_unit_high <= 0xDFFF) {
        *index += 4;
        return { true, StringView(&string, i, 4) };
    }

    *index += 2;
    return { false, StringView(&string, i, 2) };
}

String Utf16LeEncodingObject::escaped_char(unsigned char c) const {
    char buf[7];
    snprintf(buf, 7, "\\u%04llX", (long long)c);
    return String(buf);
}

nat_int_t Utf16LeEncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

nat_int_t Utf16LeEncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

String Utf16LeEncodingObject::encode_codepoint(nat_int_t codepoint) const {
    String buf;

    if (codepoint <= 0xFFFF) {
        buf.append_char(codepoint & 0xFF);
        buf.append_char(codepoint >> 8);
    } else if (codepoint <= 0x10FFFF) {
        size_t code_unit_low = ((codepoint - 0x10000) & 0x03FF) + 0xDC00;
        size_t code_unit_high = ((codepoint - 0x10000) >> 10) + 0xD800;

        buf.append_char(code_unit_high & 0xFF);
        buf.append_char(code_unit_high >> 8);
        buf.append_char(code_unit_low & 0xFF);
        buf.append_char(code_unit_low >> 8);
    } else {
        TM_UNREACHABLE();
    }

    return buf;
}

nat_int_t Utf16LeEncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 2:
        // assert(result < 0xD800 || result > 0xDFFF)

        return ((unsigned char)str[0])
            + ((unsigned char)str[1] << 8);
    case 4: {
        int high_surrogate = ((unsigned char)str[0])
            + ((unsigned char)str[1] << 8);
        int low_surrogate = ((unsigned char)str[2])
            + ((unsigned char)str[3] << 8);

        // assert(high_surrogate >= 0xD800 && high_surrogate <= 0xDBFF)
        // assert(low_surrogate >= 0xDC00 && low_surrogate <= 0xDFFF)

        return ((high_surrogate - 0xD800) << 10)
            + (low_surrogate - 0xDC00)
            + 0x10000;
    }
    default:
        return -1;
    }
}

}
