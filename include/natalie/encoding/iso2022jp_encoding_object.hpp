#pragma once

#include "natalie/encoding/dummy_encoding_object.hpp"

namespace Natalie {

class Iso2022JpEncodingObject : public DummyEncodingObject {
public:
    Iso2022JpEncodingObject()
        : DummyEncodingObject { Encoding::ISO_2022_JP, { "ISO-2022-JP", "ISO2022-JP" } } { }

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override { return codepoint; }
};

}
