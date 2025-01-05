#include "natalie/encoding/utf8_encoding_object.hpp"
#include "natalie.hpp"

namespace Natalie {

/*
    Code point ↔ UTF-8 conversion:

    First code point Last code point Byte 1   Byte 2   Byte 3   Byte 4
    U+0000           U+007F          0xxxxxxx
    U+0080           U+07FF          110xxxxx 10xxxxxx
    U+0800           U+FFFF          1110xxxx 10xxxxxx 10xxxxxx
    U+10000          U+10FFFF        11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    See: https://en.wikipedia.org/wiki/UTF-8
*/
std::tuple<bool, int, nat_int_t> Utf8EncodingObject::next_codepoint(const String &string, size_t *index) const {
    size_t len = string.size();

    if (*index >= len)
        return { true, 0, -1 };

    size_t i = *index;
    unsigned char c = string[i];

    nat_int_t codepoint = 0;
    int bytes = 0;
    if ((c >> 3) == 0b11110) {
        codepoint = (c ^ 0xF0) << 18;
        bytes = 4;
    } else if ((c >> 4) == 0b1110) {
        codepoint = (c ^ 0xE0) << 12;
        bytes = 3;
    } else if ((c >> 5) == 0b110) {
        codepoint = (c ^ 0xC0) << 6;
        bytes = 2;
    } else if ((c >> 7) == 0b0) {
        codepoint = c;
        bytes = 1;
    } else {
        *index += 1;
        return { false, 1, c };
    }

    if (bytes > 1 && i + 1 < len) {
        auto value = (unsigned char)string[i + 1];
        if (value >> 6 != 0b10) {
            *index += 1;
            return { false, 1, codepoint };
        }
        codepoint += (value ^ 0x80) << ((bytes - 2) * 6);
    }

    if (bytes > 2 && i + 2 < len) {
        auto value = (unsigned char)string[i + 2];
        if (value >> 6 != 0b10) {
            *index += 2;
            return { false, 2, codepoint };
        }
        codepoint += (value ^ 0x80) << ((bytes - 3) * 6);
    }

    if (bytes > 3 && i + 3 < len) {
        auto value = (unsigned char)string[i + 3];
        if (value >> 6 != 0b10) {
            *index += 3;
            return { false, 3, codepoint };
        }
        codepoint += value ^ 0x80;
    }

    if (*index + bytes > len) {
        bytes = len - *index;
        *index = len;
        return { false, bytes, codepoint };
    } else {
        *index += bytes;
    }

    bool valid = true;

    // Check whether a codepoint is in a valid range
    switch (bytes) {
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
            bytes = 1;
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
            bytes = 1;
        } else if (significant_bits1 == 0b1101 && (significant_bits2 >> 5) == 1) {
            valid = false;
            bytes = 1;
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
            bytes = 1; // TODO: Remove?
        }

        break;
    }
    default:
        NAT_UNREACHABLE();
    }

    return { valid, bytes, codepoint };
}

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

std::pair<bool, StringView> Utf8EncodingObject::next_char(const String &string, size_t *index) const {
    size_t i = *index;
    auto [valid, length, codepoint] = next_codepoint(string, index);

    if (!valid && length > 1) {
        // next_codepoint is greedy: invalid characters consume as many bytes as possible.
        // But String#chars and similar methods only want single bytes for invalid characters.
        // So reset the index and only consume a single byte.
        *index = i + 1;
        length = 1;
    }

    return { valid, StringView(&string, i, length) };
}

StringView Utf8EncodingObject::next_grapheme_cluster(const String &string, size_t *index) const {
    auto [valid, view] = next_char(string, index);

    bool join_next = false;
    auto index2 = *index;
    for (;;) {
        auto [valid2, length, codepoint] = next_codepoint(string, &index2);
        if (!valid2 || length == 0)
            break;

        // https://en.wikipedia.org/wiki/Variation_Selectors_(Unicode_block)
        if (codepoint >= 0xFE00 && codepoint <= 0xFE0F) {
            view = StringView { &string, view.offset(), view.size() + length };
            *index = index2;
            continue;
        }

        // Zero-width joiner
        // https://unicode-explorer.com/c/200D
        if (codepoint == 0x200D) {
            view = StringView { &string, view.offset(), view.size() + length };
            *index = index2;
            join_next = true;
            continue;
        }

        break;
    }

    if (join_next) {
        index2 = *index;
        auto [valid2, length, codepoint] = next_codepoint(string, &index2);
        if (!valid2 || codepoint == 0)
            return view;
        view = StringView { &string, view.offset(), view.size() + length };
        *index = index2;
    }

    return view;
}

/*
    0x00..0x1F, 0x7F: C0 controls (same as ASCII)
    0x80..0x9F: C1 controls
    U+FFF0..U+FFF8: non-assigned code points
    U+FFFE, U+FFFF: Not a character

    See: https://en.wikipedia.org/wiki/C0_and_C1_control_codes#Unicode
    See: https://en.wikipedia.org/wiki/Specials_(Unicode_block)
*/
bool Utf8EncodingObject::is_printable_char(const nat_int_t c) const {
    return (c >= 32 && c < 127) || (c >= 160 && c < 65520) || (c >= 65529 && c < 65534) || c >= 65536;
}

void Utf8EncodingObject::append_escaped_char(String &str, nat_int_t c) const {
    if (c <= 0xFFFF)
        str.append_sprintf("\\u%04llX", c);
    else
        str.append_sprintf("\\u{%llX}", c);
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
        buf.append_char(0xFF);
        buf.append_char(0xFD);
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
