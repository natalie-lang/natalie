#include "natalie.hpp"

namespace Natalie {

std::pair<bool, StringView> Ibm864EncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    (*index)--;
    return { true, StringView(&string, *index, 1) };
}

std::pair<bool, StringView> Ibm864EncodingObject::next_char(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, StringView() };
    size_t i = *index;
    (*index)++;
    return { true, StringView(&string, i, 1) };
}

void Ibm864EncodingObject::append_escaped_char(String &str, nat_int_t c) const {
    str.append_sprintf("\\x%02llX", c);
}

nat_int_t Ibm864EncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;
    NAT_NOT_YET_IMPLEMENTED("Conversion above Unicode Basic Latin (0x00..0x7F) not implemented");
}

nat_int_t Ibm864EncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 0x00 && codepoint <= 0x7F)
        return codepoint;
    NAT_NOT_YET_IMPLEMENTED("Conversion above Unicode Basic Latin (0x00..0x7F) not implemented");
}

String Ibm864EncodingObject::encode_codepoint(nat_int_t codepoint) const {
    return String((char)codepoint);
}

nat_int_t Ibm864EncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    default:
        return -1;
    }
}

}
