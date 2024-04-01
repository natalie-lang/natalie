#pragma once

namespace Natalie {

const size_t EncodingCount = 44;

enum class Encoding {
    ASCII_8BIT = 1,
    US_ASCII = 2,
    UTF_8 = 3,
    UTF_32LE = 4,
    UTF_32BE = 5,
    UTF_16LE = 6,
    UTF_16BE = 7,
    SHIFT_JIS = 8,
    EUC_JP = 9,
    IBM037 = 10,
    IBM437 = 11,
    IBM720 = 12,
    IBM737 = 13,
    IBM775 = 14,
    IBM850 = 15,
    IBM852 = 16,
    IBM855 = 17,
    IBM857 = 18,
    IBM860 = 19,
    IBM861 = 20,
    IBM862 = 21,
    IBM863 = 22,
    IBM864 = 23,
    IBM865 = 24,
    IBM866 = 25,
    IBM869 = 26,
    ISO_8859_1 = 27,
    ISO_8859_2 = 28,
    ISO_8859_3 = 29,
    ISO_8859_4 = 30,
    ISO_8859_5 = 31,
    ISO_8859_6 = 32,
    ISO_8859_7 = 33,
    ISO_8859_8 = 34,
    ISO_8859_9 = 35,
    ISO_8859_10 = 36,
    ISO_8859_11 = 37,
    ISO_8859_13 = 38,
    ISO_8859_14 = 39,
    ISO_8859_15 = 40,
    ISO_8859_16 = 41,
    Windows_1250 = 42,
    Windows_1251 = 43,
    Windows_1252 = 44,
};

}
