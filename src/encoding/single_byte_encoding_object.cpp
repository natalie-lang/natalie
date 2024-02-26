#include "natalie.hpp"

namespace Natalie {

std::pair<bool, StringView> SingleByteEncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    (*index)--;
    return { true, StringView(&string, *index, 1) };
}

std::pair<bool, StringView> SingleByteEncodingObject::next_char(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, StringView() };
    size_t i = *index;
    (*index)++;
    return { true, StringView(&string, i, 1) };
}

String SingleByteEncodingObject::escaped_char(const nat_int_t c) const {
    char buf[5];
    snprintf(buf, 5, "\\x%02llX", c);
    return String(buf);
}

nat_int_t SingleByteEncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;
    if (table) {
        if (codepoint >= 0x80 && codepoint <= 0xFF)
            return table[codepoint - 0x80];
        return -1;
    }
    NAT_NOT_YET_IMPLEMENTED("Conversion above Unicode Basic Latin (0x00..0x7F) not implemented");
}

nat_int_t SingleByteEncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;
    if (table) {
        for (long i = 0; i <= 127; i++) {
            if (table[i] == codepoint)
                return i + 0x80;
        }
        return -1;
    }
    NAT_NOT_YET_IMPLEMENTED("Conversion above Unicode Basic Latin (0x00..0x7F) not implemented");
}

String SingleByteEncodingObject::encode_codepoint(nat_int_t codepoint) const {
    return String((char)codepoint);
}

nat_int_t SingleByteEncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    default:
        return -1;
    }
}

}
