#pragma once

#include "natalie/encoding/dummy_encoding_object.hpp"

namespace Natalie {

class Utf32EncodingObject : public DummyEncodingObject {
public:
    Utf32EncodingObject()
        : DummyEncodingObject { Encoding::UTF_32, { "UTF-32" } } { }

    virtual Value encode(Env *, EncodingObject *, StringObject *, EncodeOptions) const override;
};

}
