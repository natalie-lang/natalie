#include "natalie.hpp"

namespace Natalie {

String Utf8EncodingObject::next_char(Env *env, const String &string, size_t *index) const {
    size_t len = string.size();
    if (*index >= len)
        return String();
    char buffer[5];
    size_t i = *index;
    int length = 0;
    buffer[0] = string[i];
    if (((unsigned char)buffer[0] >> 3) == 30) { // 11110xxx, 4 bytes
        if (i + 3 >= len) raise_encoding_invalid_byte_sequence_error(env, string, i);
        buffer[1] = string[++i];
        buffer[2] = string[++i];
        buffer[3] = string[++i];
        buffer[4] = 0;
        length = 4;
    } else if (((unsigned char)buffer[0] >> 4) == 14) { // 1110xxxx, 3 bytes
        if (i + 2 >= len) raise_encoding_invalid_byte_sequence_error(env, string, i);
        buffer[1] = string[++i];
        buffer[2] = string[++i];
        buffer[3] = 0;
        length = 3;
    } else if (((unsigned char)buffer[0] >> 5) == 6) { // 110xxxxx, 2 bytes
        if (i + 1 >= len) raise_encoding_invalid_byte_sequence_error(env, string, i);
        buffer[1] = string[++i];
        buffer[2] = 0;
        length = 2;
    } else {
        buffer[1] = 0;
        length = 1;
    }
    *index = i + 1;
    return String(buffer, length);
}
String Utf8EncodingObject::escaped_char(unsigned char c) const {
    char buf[7];
    snprintf(buf, 7, "\\u%04llX", (long long)c);
    return String(buf);
}

Value Utf8EncodingObject::encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const {
    switch (orig_encoding->num()) {
    case Encoding::UTF_8:
        // nothing to do
        return str;
    case Encoding::ASCII_8BIT:
    case Encoding::US_ASCII:
        // TODO: some sort of conversion?
        str->set_encoding(EncodingObject::get(num()));
        return str;
    default:
        ClassObject *EncodingClass = find_top_level_const(env, "Encoding"_s)->as_class();
        env->raise(EncodingClass->const_find(env, "ConverterNotFoundError"_s)->as_class(), "code converter not found");
    }
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

}
