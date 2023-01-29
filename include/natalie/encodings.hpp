#pragma once

namespace Natalie {

const int EncodingCount = 7;

enum class Encoding {
    ASCII_8BIT = 1,
    US_ASCII = 2,
    UTF_8 = 3,
    UTF_32LE = 4,
    UTF_32BE = 5,
    UTF_16LE = 6,
    UTF_16BE = 7,
};

}
