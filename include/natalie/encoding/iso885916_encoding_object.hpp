#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding/single_byte_encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Iso885916EncodingObject : public SingleByteEncodingObject {
public:
    Iso885916EncodingObject()
        : SingleByteEncodingObject { Encoding::ISO_8859_16, { "ISO-8859-16", "ISO8859-16" } } { }

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override;
    virtual nat_int_t from_unicode_codepoint(nat_int_t codepoint) const override;
};

}
