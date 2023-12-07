#include "natalie.hpp"

namespace Natalie {

nat_int_t Iso88591EncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

nat_int_t Iso88591EncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

}
