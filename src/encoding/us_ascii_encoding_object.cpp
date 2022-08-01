#include "natalie.hpp"

namespace Natalie {

StringView UsAsciiEncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return StringView();
    (*index)--;
    unsigned char c = string[*index];
    if ((int)c > 127)
        raise_encoding_invalid_byte_sequence_error(string, *index);
    return StringView(&string, *index, 1);
}

StringView UsAsciiEncodingObject::next_char(const String &string, size_t *index) const {
    if (*index >= string.size())
        return StringView();
    size_t i = *index;
    unsigned char c = string[i];
    if ((int)c > 127)
        raise_encoding_invalid_byte_sequence_error(string, i);
    (*index)++;
    return StringView(&string, i, 1);
}

String UsAsciiEncodingObject::escaped_char(unsigned char c) const {
    char buf[5];
    snprintf(buf, 5, "\\x%02llX", (long long)c);
    return String(buf);
}

Value UsAsciiEncodingObject::encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const {
    ClassObject *EncodingClass = find_top_level_const(env, "Encoding"_s)->as_class();
    switch (orig_encoding->num()) {
    case Encoding::ASCII_8BIT: {
        auto string = str->string();
        for (size_t i = 0; i < string.size(); ++i) {
            unsigned char c = string[i];
            if (!valid_codepoint(c)) {
                Value ord = Value::integer(c);
                auto message = StringObject::format("U+{} from UTF-8 to US-ASCII", String::hex(ord->as_integer()->to_nat_int_t(), String::HexFormat::Uppercase));
                env->raise(EncodingClass->const_find(env, "UndefinedConversionError"_s)->as_class(), message);
            }
        }
        str->set_encoding(EncodingObject::get(num()));
        return str;
    }
    case Encoding::US_ASCII:
        // nothing to do
        return str;
    case Encoding::UTF_8: {
        ArrayObject *chars = str->chars(env);
        for (size_t i = 0; i < chars->size(); i++) {
            StringObject *char_obj = (*chars)[i]->as_string();
            if (char_obj->length() > 1) {
                Value ord = char_obj->ord(env);
                auto message = StringObject::format("U+{} from UTF-8 to US-ASCII", String::hex(ord->as_integer()->to_nat_int_t(), String::HexFormat::Uppercase));
                env->raise(EncodingClass->const_find(env, "UndefinedConversionError"_s)->as_class(), message);
            }
        }
        str->set_encoding(EncodingObject::get(num()));
        return str;
    }
    default:
        env->raise(EncodingClass->const_find(env, "ConverterNotFoundError"_s)->as_class(), "code converter not found");
    }
}

String UsAsciiEncodingObject::encode_codepoint(nat_int_t codepoint) const {
    return String((char)codepoint);
}

}
