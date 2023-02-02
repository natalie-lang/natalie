#pragma once

namespace Natalie {

const int EncodingCount = 12;

enum class Encoding {
    ASCII_8BIT = 1,
    US_ASCII = 2,
    UTF_8 = 3,
    UTF_32LE = 4,
    UTF_32BE = 5,
    UTF_16LE = 6,
    UTF_16BE = 7,
    IBM866 = 8,
    IBM437 = 9,
    SHIFT_JIS = 10,
    EUC_JP = 11,
    ISO_8859_1 = 12,
};

}
