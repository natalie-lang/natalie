#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Ascii8BitEncodingObject : public EncodingObject {
public:
    Ascii8BitEncodingObject()
        : EncodingObject { Encoding::ASCII_8BIT, { "ASCII-8BIT", "BINARY" } } { }

    virtual String next_char(Env *env, String &string, size_t *index) const override {
        if (*index >= string.size())
            return String();
        char buffer[2];
        size_t i = *index;
        buffer[0] = string[i];
        buffer[1] = 0;
        (*index)++;
        return String(buffer, 1);
    }

    virtual String escaped_char(unsigned char c) const override {
        char buf[5];
        snprintf(buf, 5, "\\x%02llX", (long long)c);
        return String(buf);
    }

    virtual Value encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const override;
};

}
