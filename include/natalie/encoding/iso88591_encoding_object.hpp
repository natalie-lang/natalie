#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding/single_byte_encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Iso88591EncodingObject : public SingleByteEncodingObject {
public:
    Iso88591EncodingObject()
        : SingleByteEncodingObject { Encoding::ISO_8859_1, { "ISO-8859-1", "ISO8859-1" }, nullptr } { }

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const override;
};

}
