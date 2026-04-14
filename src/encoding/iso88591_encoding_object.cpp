#include "natalie.hpp"

namespace Natalie {

nat_int_t Iso88591EncodingObject::to_unicode_codepoint(nat_int_t codepoint) const {
    return codepoint;
}

nat_int_t Iso88591EncodingObject::from_unicode_codepoint(nat_int_t codepoint) const {
    if (codepoint < 0 || codepoint > 0xFF)
        return -1;
    return codepoint;
}

}
