#pragma once

#include <stddef.h>

namespace Natalie {

enum class Encoding : size_t {
    NONE,
    ASCII_8BIT,
    US_ASCII,
    UTF_8,
    UTF_32LE,
    UTF_32BE,
    UTF_16LE,
    UTF_16BE,
    SHIFT_JIS,
    EUC_JP,
    IBM037,
    IBM437,
    IBM720,
    IBM737,
    IBM775,
    IBM850,
    IBM852,
    IBM855,
    IBM857,
    IBM860,
    IBM861,
    IBM862,
    IBM863,
    IBM864,
    IBM865,
    IBM866,
    IBM869,
    ISO_8859_1,
    ISO_8859_2,
    ISO_8859_3,
    ISO_8859_4,
    ISO_8859_5,
    ISO_8859_6,
    ISO_8859_7,
    ISO_8859_8,
    ISO_8859_9,
    ISO_8859_10,
    ISO_8859_11,
    ISO_8859_13,
    ISO_8859_14,
    ISO_8859_15,
    ISO_8859_16,
    Windows_1250,
    Windows_1251,
    Windows_1252,
    Windows_1253,
    Windows_1254,
    Windows_1258,
    TERMINATOR, // Keep this as the last entry
};

constexpr size_t EncodingCount = static_cast<size_t>(Encoding::TERMINATOR) - 1;

}
