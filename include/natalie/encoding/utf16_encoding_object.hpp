#pragma once

#include "natalie/encoding/dummy_encoding_object.hpp"

namespace Natalie {

class Utf16EncodingObject : public DummyEncodingObject {
public:
    Utf16EncodingObject()
        : DummyEncodingObject { Encoding::UTF_16, { "UTF-16" } } { }
};

}
