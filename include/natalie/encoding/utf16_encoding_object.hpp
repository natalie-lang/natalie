#pragma once

#include "natalie/encoding/dummy_encoding_object.hpp"

namespace Natalie {

class Utf16EncodingObject : public DummyEncodingObject {
public:
    Utf16EncodingObject()
        : DummyEncodingObject { Encoding::UTF_16, { "UTF-16" } } { }

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override { return codepoint; }
    virtual std::tuple<bool, int, nat_int_t> next_codepoint(const String &, size_t *) const override;
};

}
