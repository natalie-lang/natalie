#include "natalie.hpp"

namespace Natalie {

static const long ISO885913[] = {
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93,
    0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
    0x9E, 0x9F, 0xA0, 0x201D, 0xA2, 0xA3, 0xA4, 0x201E, 0xA6, 0xA7,
    0xD8, 0xA9, 0x156, 0xAB, 0xAC, 0xAD, 0xAE, 0xC6, 0xB0, 0xB1,
    0xB2, 0xB3, 0x201C, 0xB5, 0xB6, 0xB7, 0xF8, 0xB9, 0x157, 0xBB,
    0xBC, 0xBD, 0xBE, 0xE6, 0x104, 0x12E, 0x100, 0x106, 0xC4, 0xC5,
    0x118, 0x112, 0x10C, 0xC9, 0x179, 0x116, 0x122, 0x136, 0x12A, 0x13B,
    0x160, 0x143, 0x145, 0xD3, 0x14C, 0xD5, 0xD6, 0xD7, 0x172, 0x141,
    0x15A, 0x16A, 0xDC, 0x17B, 0x17D, 0xDF, 0x105, 0x12F, 0x101, 0x107,
    0xE4, 0xE5, 0x119, 0x113, 0x10D, 0xE9, 0x17A, 0x117, 0x123, 0x137,
    0x12B, 0x13C, 0x161, 0x144, 0x146, 0xF3, 0x14D, 0xF5, 0xF6, 0xF7,
    0x173, 0x142, 0x15B, 0x16B, 0xFC, 0x17C, 0x17E, 0x2019
};

Iso885913EncodingObject::Iso885913EncodingObject()
    : SingleByteEncodingObject { Encoding::ISO_8859_13, { "ISO-8859-13", "ISO8859-13" }, ISO885913 } { }

}
