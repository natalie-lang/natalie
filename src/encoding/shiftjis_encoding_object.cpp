#include "natalie/encoding/shiftjis_encoding_object.hpp"
#include "natalie.hpp"
#include "natalie/ioutil.hpp"

namespace Natalie {

bool ShiftJisEncodingObject::valid_codepoint(nat_int_t codepoint) const {
    // ascii
    if (codepoint >= 0 && codepoint <= 0x7F)
        return true;

    // half-width Katakana
    if (codepoint >= 0xA1 && codepoint <= 0xDF)
        return true;

    auto b1 = (codepoint & 0xFF00) >> 8;
    if (!((b1 >= 0x81 && b1 <= 0x9F) || (b1 >= 0xE0 && b1 <= 0xEF)))
        return false;

    auto b2 = (codepoint & 0x00FF);
    return b2 >= 0x40 && b2 <= 0xFC && b2 != 0x7F;
}

std::pair<bool, StringView> ShiftJisEncodingObject::prev_char(const String &string, size_t *index) const {
    NAT_NOT_YET_IMPLEMENTED();
}

std::pair<bool, StringView> ShiftJisEncodingObject::next_char(const String &string, size_t *index) const {
    size_t len = string.size();
    if (*index >= len)
        return { true, StringView() };
    size_t i = *index;
    int length = 0;
    unsigned char c = string[i];
    // Check the first byte and determine length.
    // Note that the lead byte can go up to 0xFC per JIS X 2013 extensions
    if ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC)) {
        // first byte of two byte case, so expect another byte
        if (i + 1 >= len) { // missing 2nd byte, this is bad.
            *index = len;
            return { false, StringView(&string, i) };
        }
        // if we got here, it's a two-byte sequence.
        unsigned char c2 = string[i + 1];
        bool valid_2nd_byte = (c2 >= 0x40 && c2 != 0x7F && c2 <= 0xFC);
        if (valid_2nd_byte) {
            *index += 2;
            return { true, StringView(&string, i, 2) };
        } else {
            *index += 1;
            return { false, StringView(&string, i, 1) };
        }
    } else { // is a single byte char
        // first char above 0x80 but not in half-width katakana must be invalid
        bool valid_1st_byte = (c <= 0x7F || (c >= 0xA1 && c <= 0xDF));
        *index += 1;
        return { valid_1st_byte, StringView(&string, i, 1) };
    }
}

void ShiftJisEncodingObject::append_escaped_char(String &str, nat_int_t c) const {
    if (c >= 0 && c <= 0xFF) {
        str.append_sprintf("\\x%02llX", c);
    } else {
        str.append_sprintf("\\u%04llX", c);
    }
}

// https://encoding.spec.whatwg.org/#shift_jis-decoder
nat_int_t ShiftJisEncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    // 4. If byte is an ASCII byte or 0x80, return a code point whose value is byte.
    if ((codepoint >= 0x00 && codepoint <= 0x7F) || codepoint == 0x80)
        return codepoint;

    // Special cases to match CRuby (not in WHATWG spec):
    switch (codepoint) {
    case 0x815c:
        return 0x2014;
    case 0x8160:
        return 0x301c;
    case 0x8161:
        return 0x2016;
    case 0x817c:
        return 0x2212;
    case 0x8191:
        return 0xa2;
    case 0x8192:
        return 0xa3;
    case 0x81ca:
        return 0xac;
    }
    if (codepoint >= 0x8740 && codepoint <= 0x879c)
        return -1;

    // 5. If byte is in the range 0xA1 to 0xDF, inclusive,
    // return a code point whose value is 0xFF61 − 0xA1 + byte.
    if (codepoint >= 0xA1 && codepoint <= 0xDF)
        return 0xFF61 - 0xA1 + codepoint;

    unsigned char b1 = (codepoint & 0xFF00) >> 8;
    unsigned char b2 = (codepoint & 0xFF);

    // 6. If byte is in the range 0x81 to 0x9F, inclusive, or 0xE0 to 0xFC, inclusive,
    // set Shift_JIS lead to byte and return continue.
    if ((b1 >= 0x81 && b1 <= 0x9F) || (b1 >= 0xE0 && b1 <= 0xFC)) {
        // 3. If Shift_JIS lead is not 0x00, let lead be Shift_JIS lead,
        // let pointer be null, set Shift_JIS lead to 0x00, and then:

        int lead = b1;
        int pointer = -1;

        // 3.1. Let offset be 0x40 if byte is less than 0x7F, otherwise 0x41.
        int offset = b2 < 0x7F ? 0x40 : 0x41;

        // 3.2. Let lead offset be 0x81 if lead is less than 0xA0, otherwise 0xC1.
        int lead_offset = lead < 0xA0 ? 0x81 : 0xC1;

        // 3.3. If byte is in the range 0x40 to 0x7E, inclusive, or 0x80 to 0xFC, inclusive,
        // set pointer to (lead − lead offset) × 188 + byte − offset.
        if ((b2 >= 0x40 && b2 <= 0x7E) || (b2 >= 0x80 && b2 <= 0xFC))
            pointer = (lead - lead_offset) * 188 + b2 - offset;

        // 3.4. If pointer is in the range 8836 to 10715, inclusive,
        // return a code point whose value is 0xE000 − 8836 + pointer.
        if (pointer >= 8836 && pointer <= 10715)
            return 0xE000 - 8836 + pointer;

        // 3.5. Let code point be null if pointer is null,
        // otherwise the index code point for pointer in index jis0208.
        if (pointer != -1) {
            int cp = -1;
            if (pointer >= 0 && pointer <= JIS0208_max) {
                cp = JIS0208[pointer];
            }

            // 3.6. If code point is non-null, return a code point whose value is code point.
            if (cp != -1)
                return cp;

            // 3.7. If byte is an ASCII byte, restore byte to ioQueue.
            // NOTE: not possible with our implementation
        }

        // 3.8. Return error.
        return -1;
    }

    // 7. Return error.
    return -1;
}

nat_int_t ShiftJisEncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;
    NAT_NOT_YET_IMPLEMENTED("Conversion above Unicode Basic Latin (0x00..0x7F) not implemented");
}

String ShiftJisEncodingObject::encode_codepoint(nat_int_t codepoint) const {
    String buf;
    if (codepoint > 0xff) {
        buf.append_char((codepoint & 0xff00) >> 8);
        buf.append_char(codepoint & 0xff);
    } else {
        buf.append_char(codepoint);
    }
    return buf;
}

nat_int_t ShiftJisEncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    case 2:
        return ((unsigned char)str[0] << 8)
            + ((unsigned char)str[1]);
    default:
        return -1;
    }
}

}
