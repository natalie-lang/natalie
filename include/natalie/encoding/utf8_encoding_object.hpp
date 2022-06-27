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

    virtual bool valid_codepoint(nat_int_t codepoint) const override {
        // TODO: there are some ranges in here that are invalid
        return codepoint >= 0 && codepoint <= 0x10FFFF;
    }

    virtual String next_char(Env *env, String &string, size_t *index) const override;

    virtual String escaped_char(unsigned char c) const override;

    virtual Value encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const override;
};

}
