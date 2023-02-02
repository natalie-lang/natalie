#include "natalie.hpp"

namespace Natalie {

std::pair<bool, StringView> Ibm437EncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    (*index)--;
    return { true, StringView(&string, *index, 1) };
}

std::pair<bool, StringView> Ibm437EncodingObject::next_char(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, StringView() };
    size_t i = *index;
    (*index)++;
    return { true, StringView(&string, i, 1) };
}

String Ibm437EncodingObject::escaped_char(unsigned char c) const {
    char buf[5];
    snprintf(buf, 5, "\\x%02llX", (long long)c);
    return String(buf);
}

nat_int_t Ibm437EncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 128)
        return -1;

    return codepoint;
}

nat_int_t Ibm437EncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint >= 128)
        return -1;

    return codepoint;
}

String Ibm437EncodingObject::encode_codepoint(nat_int_t codepoint) const {
    return String((char)codepoint);
}

nat_int_t Ibm437EncodingObject::decode_codepoint(StringView &str) const {
    switch (str.size()) {
    case 1:
        return (unsigned char)str[0];
    default:
        return -1;
    }
}

}
