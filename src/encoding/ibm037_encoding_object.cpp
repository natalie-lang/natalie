#include "natalie.hpp"

// IBM037 (CP37) is not an ASCII based encoding
// See: https://en.wikibooks.org/wiki/Character_Encodings/Code_Tables/EBCDIC/EBCDIC_037

namespace Natalie {

std::pair<bool, StringView> Ibm037EncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    (*index)--;
    return { true, StringView(&string, *index, 1) };
}

std::pair<bool, StringView> Ibm037EncodingObject::next_char(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, StringView() };
    size_t i = *index;
    (*index)++;
    return { true, StringView(&string, i, 1) };
}

void Ibm037EncodingObject::append_escaped_char(String &str, nat_int_t c) const {
    str.append_sprintf("\\x%02llX", c);
}

nat_int_t Ibm037EncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    NAT_NOT_YET_IMPLEMENTED("Conversion of EBCDIC not implemented");
}

nat_int_t Ibm037EncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    NAT_NOT_YET_IMPLEMENTED("Conversion of EBCDIC not implemented");
}

String Ibm037EncodingObject::encode_codepoint(nat_int_t codepoint) const {
    return String((char)codepoint);
}

nat_int_t Ibm037EncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    default:
        return -1;
    }
}

}
