#include "natalie.hpp"

namespace Natalie {

static const long ISO88594[] = {
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93,
    0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
    0x9E, 0x9F, 0xA0, 0x104, 0x138, 0x156, 0xA4, 0x128, 0x13B, 0xA7,
    0xA8, 0x160, 0x112, 0x122, 0x166, 0xAD, 0x17D, 0xAF, 0xB0, 0x105,
    0x2DB, 0x157, 0xB4, 0x129, 0x13C, 0x2C7, 0xB8, 0x161, 0x113, 0x123,
    0x167, 0x14A, 0x17E, 0x14B, 0x100, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5,
    0xC6, 0x12E, 0x10C, 0xC9, 0x118, 0xCB, 0x116, 0xCD, 0xCE, 0x12A,
    0x110, 0x145, 0x14C, 0x136, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0x172,
    0xDA, 0xDB, 0xDC, 0x168, 0x16A, 0xDF, 0x101, 0xE1, 0xE2, 0xE3,
    0xE4, 0xE5, 0xE6, 0x12F, 0x10D, 0xE9, 0x119, 0xEB, 0x117, 0xED,
    0xEE, 0x12B, 0x111, 0x146, 0x14D, 0x137, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0x173, 0xFA, 0xFB, 0xFC, 0x169, 0x16B, 0x2D9
};

Iso88594EncodingObject::Iso88594EncodingObject()
    : SingleByteEncodingObject { Encoding::ISO_8859_4, { "ISO-8859-4", "ISO8859-4" }, ISO88594 } { }

}
