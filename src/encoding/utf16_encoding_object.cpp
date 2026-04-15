#include "natalie.hpp"

namespace Natalie {

std::tuple<bool, int, nat_int_t> Utf16EncodingObject::next_codepoint(const String &string, size_t *index) const {
    if (*index >= string.size())
        return { true, 0, 0 };

    if (*index == 0 && string.size() >= 2) {
        unsigned char b0 = string[0];
        unsigned char b1 = string[1];

        if ((b0 == 0xFE && b1 == 0xFF) || (b0 == 0xFF && b1 == 0xFE))
            *index = 2;
    }

    if (string.size() >= 2) {
        unsigned char b0 = string[0];
        unsigned char b1 = string[1];
        if ((b0 == 0xFE && b1 == 0xFF) || (b0 == 0xFF && b1 == 0xFE)) {
            auto *delegate = (b0 == 0xFE && b1 == 0xFF)
                ? EncodingObject::get(Encoding::UTF_16BE)
                : EncodingObject::get(Encoding::UTF_16LE);
            return delegate->next_codepoint(string, index);
        }
    }

    return DummyEncodingObject::next_codepoint(string, index);
}

}
