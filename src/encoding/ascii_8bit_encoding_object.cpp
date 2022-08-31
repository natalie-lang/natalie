#include "natalie.hpp"

namespace Natalie {

std::pair<bool, StringView> Ascii8BitEncodingObject::prev_char(const String &string, size_t *index) const {
    if (*index == 0)
        return { true, StringView() };
    (*index)--;
    return { true, StringView(&string, *index, 1) };
}

std::pair<bool, StringView> Ascii8BitEncodingObject::next_char(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, StringView() };
    size_t i = *index;
    (*index)++;
    return { true, StringView(&string, i, 1) };
}

String Ascii8BitEncodingObject::escaped_char(unsigned char c) const {
    char buf[5];
    snprintf(buf, 5, "\\x%02llX", (long long)c);
    return String(buf);
}

Value Ascii8BitEncodingObject::encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const {
    ClassObject *EncodingClass = find_top_level_const(env, "Encoding"_s)->as_class();
    switch (orig_encoding->num()) {
    case Encoding::ASCII_8BIT:
        // nothing to do
        return str;
    case Encoding::US_ASCII:
        str->set_encoding(EncodingObject::get(num()));
        return str;
    case Encoding::UTF_8: {
        ArrayObject *chars = str->chars(env)->as_array();
        for (size_t i = 0; i < chars->size(); i++) {
            StringObject *char_obj = (*chars)[i]->as_string();
            if (char_obj->length() > 1) {
                Value ord = char_obj->ord(env);
                auto message = StringObject::format("U+{} from UTF-8 to ASCII-8BIT", String::hex(ord->as_integer()->to_nat_int_t(), String::HexFormat::Uppercase));
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

String Ascii8BitEncodingObject::encode_codepoint(nat_int_t codepoint) const {
    return String((char)codepoint);
}

}
