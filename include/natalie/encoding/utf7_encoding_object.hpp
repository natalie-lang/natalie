#pragma once

#include "natalie/encoding/dummy_encoding_object.hpp"

namespace Natalie {

class Utf7EncodingObject : public DummyEncodingObject {
public:
    Utf7EncodingObject()
        : DummyEncodingObject { Encoding::UTF_7, { "UTF-7", "CP65000" } } { }

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override { return codepoint; }
};

}
