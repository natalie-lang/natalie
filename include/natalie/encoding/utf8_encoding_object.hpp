#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Utf8EncodingObject : public EncodingObject {
public:
    Utf8EncodingObject()
        : EncodingObject { Encoding::UTF_8, { "UTF-8" } } { }

    virtual String next_char(Env *env, String &string, size_t *index) const override {
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

    virtual String escaped_char(unsigned char c) const override {
        char buf[7];
        snprintf(buf, 7, "\\u%04llX", (long long)c);
        return String(buf);
    }

    virtual Value encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const override;
};

}
