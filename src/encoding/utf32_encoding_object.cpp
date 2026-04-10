#include "natalie.hpp"

namespace Natalie {

std::tuple<bool, int, nat_int_t> Utf32EncodingObject::next_codepoint(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, 0, 0 };

    if (*index == 0 && string.size() >= 4) {
        unsigned char b0 = string[0];
        unsigned char b1 = string[1];
        unsigned char b2 = string[2];
        unsigned char b3 = string[3];

        if ((b0 == 0x00 && b1 == 0x00 && b2 == 0xFE && b3 == 0xFF) || (b0 == 0xFF && b1 == 0xFE && b2 == 0x00 && b3 == 0x00))
            *index = 4;
    }

    if (string.size() >= 4) {
        unsigned char b0 = string[0];
        unsigned char b1 = string[1];
        unsigned char b2 = string[2];
        unsigned char b3 = string[3];
        if ((b0 == 0x00 && b1 == 0x00 && b2 == 0xFE && b3 == 0xFF) || (b0 == 0xFF && b1 == 0xFE && b2 == 0x00 && b3 == 0x00)) {
            auto *delegate = (b0 == 0x00 && b1 == 0x00)
                ? EncodingObject::get(Encoding::UTF_32BE)
                : EncodingObject::get(Encoding::UTF_32LE);
            return delegate->next_codepoint(string, index);
        }
    }

    return DummyEncodingObject::next_codepoint(string, index);
}

Value Utf32EncodingObject::encode(Env *env, EncodingObject *orig_encoding, StringObject *str, EncodeOptions options) const {
    auto *delegate = EncodingObject::get(Encoding::UTF_32BE);
    delegate->encode(env, orig_encoding, str, options);
    if (str->bytesize() > 0) {
        auto with_bom = String("\x00\x00\xFE\xFF", 4);
        with_bom.append(str->string());
        str->set_str(std::move(with_bom));
    }
    str->set_encoding(EncodingObject::get(num()));
    return str;
}

}
