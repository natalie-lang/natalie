#include "natalie.hpp"

namespace Natalie {

std::pair<bool, StringView> Utf8EncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    size_t length = 1;
    (*index)--;
    unsigned char c = string[*index];
    if ((int)c <= 127)
        return { true, StringView(&string, *index, 1) };
    while ((c >> 6) != 0b11) { // looking for 11xxxxxx
        if (*index == 0)
            return { false, StringView() };
        (*index)--;
        length++;
        if (length > 4) {
            *index += 4;
            return { false, StringView(&string, *index, 1) };
        }
        c = string[*index];
    }
    return { true, StringView(&string, *index, length) };
}

/*
    Code point ↔ UTF-8 conversion:

    First code point Last code point Byte 1   Byte 2   Byte 3   Byte 4
    U+0000           U+007F          0xxxxxxx
    U+0080           U+07FF          110xxxxx 10xxxxxx
    U+0800           U+FFFF          1110xxxx 10xxxxxx 10xxxxxx
    U+10000          U+10FFFF        11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    See: https://en.wikipedia.org/wiki/UTF-8
*/
std::pair<bool, StringView> Utf8EncodingObject::next_char(const String &string, size_t *index) const {
    size_t len = string.size();

    if (*index >= len)
        return { true, StringView() };

    size_t i = *index;
    int length = 0;
    unsigned char c = string[i];
    bool valid = true;

    // Check the first byte and determine length
    if ((c >> 3) == 0b11110) { // 11110xxx, 4 bytes
        if (i + 3 >= len) {
            *index = len;
            return { false, StringView(&string, i) };
        }
        length = 4;
    } else if ((c >> 4) == 0b1110) { // 1110xxxx, 3 bytes
        if (i + 2 >= len) {
            *index = len;
            return { false, StringView(&string, i) };
        }
        length = 3;
    } else if ((c >> 5) == 0b110) { // 110xxxxx, 2 bytes
        if (i + 1 >= len) {
            *index = len;
            return { false, StringView(&string, i) };
        }
        length = 2;
    } else if ((c >> 7) == 0b0) { // 0xxxxxxx, 1 byte
        length = 1;
    } else {
        *index += 1;
        return { false, StringView(&string, i, 1) };
    }

    // All, but the 1st, bytes should match the format 10xxxxxx
    for (size_t j = i + 1; j <= i + length - 1; j++) {
        unsigned char cj = string[j];
        if (cj >> 6 != 0b10) {
            *index += 1;
            return { false, StringView(&string, i, 1) };
        }
    }

    // Check whether a codepoint is in a valid range
    switch (length) {
    case 1:
        // no check
        // all values that can be represented with 7 bits (0-127) are correct
        break;
    case 2: {
        // Codepoints range: U+0080..U+07FF
        // Check the highest 4 significant bits of
        // 110xxxx-	10------
        unsigned char extra_bits = (unsigned char)string[i] & 0b11110;

        if (extra_bits == 0) {
            valid = false;
            length = 1;
        }

        break;
    }
    case 3: {
        // Codepoints range: U+0800..U+FFFF.
        // Check the highest 5 significant bits of
        // 1110xxxx	10x-----	10------
        //
        // U+D800..U+DFFF - invalid codepoints
        // xxxx1101 xx1----- xx------
        unsigned char extra_bits1 = (unsigned char)string[i] & 0b1111;
        unsigned char extra_bits2 = (unsigned char)string[i + 1] & 0b100000;
        unsigned char significant_bits1 = (unsigned char)string[i] & 0b1111;
        unsigned char significant_bits2 = (unsigned char)string[i + 1] & 0b111111;

        if (extra_bits1 == 0 && extra_bits2 == 0) {
            valid = false;
            length = 1;
        } else if (significant_bits1 == 0b1101 && (significant_bits2 >> 5) == 1) {
            valid = false;
            length = 1;
        }

        break;
    }
    case 4: {
        // Codepoints range: U+10000..U+10FFFF.
        // 11110xxx	10xxxxxx 10xxxxxx 10xxxxxx
        unsigned char significant_bits1 = (unsigned char)string[i] & 0b111;
        unsigned char significant_bits2 = (unsigned char)string[i + 1] & 0b111111;
        unsigned char significant_bits3 = (unsigned char)string[i + 2] & 0b111111;
        unsigned char significant_bits4 = (unsigned char)string[i + 3] & 0b111111;

        int codepoint = significant_bits4 | (significant_bits3 << 6) | (significant_bits2 << 12) | (significant_bits1 << 18);

        if (codepoint < 0x10000 || codepoint > 0x10FFFF) {
            valid = false;
            length = 1;
        }

        break;
    }
    }

    *index += length;
    return { valid, StringView(&string, i, length) };
}

String Utf8EncodingObject::escaped_char(unsigned char c) const {
    char buf[7];
    snprintf(buf, 7, "\\u%04llX", (long long)c);
    return String(buf);
}

nat_int_t Utf8EncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

nat_int_t Utf8EncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

// public domain
// https://gist.github.com/Miouyouyou/864130e8734afe3f806512b14022226f
String Utf8EncodingObject::encode_codepoint(nat_int_t codepoint) const {
    String buf;
    if (codepoint < 0x80) {
        buf.append_char(codepoint);
    } else if (codepoint < 0x800) { // 00000yyy yyxxxxxx
        buf.append_char(0b11000000 | (codepoint >> 6));
        buf.append_char(0b10000000 | (codepoint & 0x3f));
    } else if (codepoint < 0x10000) { // zzzzyyyy yyxxxxxx
        buf.append_char(0b11100000 | (codepoint >> 12));
        buf.append_char(0b10000000 | ((codepoint >> 6) & 0x3f));
        buf.append_char(0b10000000 | (codepoint & 0x3f));
    } else if (codepoint < 0x200000) { // 000uuuuu zzzzyyyy yyxxxxxx
        buf.append_char(0b11110000 | (codepoint >> 18));
        buf.append_char(0b10000000 | ((codepoint >> 12) & 0x3f));
        buf.append_char(0b10000000 | ((codepoint >> 6) & 0x3f));
        buf.append_char(0b10000000 | (codepoint & 0x3f));
    } else {
        TM_UNREACHABLE();
    }
    return buf;
}

nat_int_t Utf8EncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    case 2:
        return (((unsigned char)str[0] ^ 0xC0) << 6)
            + (((unsigned char)str[1] ^ 0x80) << 0);
    case 3:
        return (((unsigned char)str[0] ^ 0xE0) << 12)
            + (((unsigned char)str[1] ^ 0x80) << 6)
            + (((unsigned char)str[2] ^ 0x80) << 0);
    case 4:
        return (((unsigned char)str[0] ^ 0xF0) << 18)
            + (((unsigned char)str[1] ^ 0x80) << 12)
            + (((unsigned char)str[2] ^ 0x80) << 6)
            + (((unsigned char)str[3] ^ 0x80) << 0);
    default:
        return -1;
    }
}

}
