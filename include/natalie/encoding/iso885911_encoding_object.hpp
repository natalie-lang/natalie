#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/encoding/single_byte_encoding_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

using namespace TM;

class Iso885911EncodingObject : public SingleByteEncodingObject {
public:
    Iso885911EncodingObject();
};

}
