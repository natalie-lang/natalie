#include "natalie.hpp"

namespace Natalie {

Iso885915EncodingObject::Iso885915EncodingObject()
    : SingleByteEncodingObject { Encoding::ISO_8859_15, { "ISO-8859-15", "ISO8859-15" }, nullptr } { }

}
