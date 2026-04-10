#pragma once

#include "natalie/encoding/dummy_encoding_object.hpp"

namespace Natalie {

class Utf32EncodingObject : public DummyEncodingObject {
public:
    Utf32EncodingObject()
        : DummyEncodingObject { Encoding::UTF_32, { "UTF-32" } } { }

    virtual nat_int_t to_unicode_codepoint(nat_int_t codepoint) const override { return codepoint; }
    virtual Value encode(Env *, EncodingObject *, StringObject *, EncodeOptions) const override;
    virtual std::tuple<bool, int, nat_int_t> next_codepoint(const String &, size_t *) const override;
};

}
