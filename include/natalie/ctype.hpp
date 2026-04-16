#pragma once

// ASCII-only ctype helpers. Unlike the standard <ctype.h> functions, these
// are locale-independent and safe to call with any int value: only bytes in
// the ASCII range (0-127) can match. This matches MRI's ISPRINT/ISSPACE/etc.
// macros (include/ruby/internal/ctype.h), which exist for the same reasons:
// locale independence and sign-extension safety.

namespace Natalie {

constexpr bool is_ascii(int c) {
    return (unsigned)c <= 0x7f;
}

constexpr bool is_ascii_printable(int c) {
    return c >= 0x20 && c <= 0x7e;
}

constexpr bool is_ascii_space(int c) {
    return c == ' ' || (c >= '\t' && c <= '\r');
}

constexpr bool is_ascii_digit(int c) {
    return c >= '0' && c <= '9';
}

constexpr bool is_ascii_upper(int c) {
    return c >= 'A' && c <= 'Z';
}

constexpr bool is_ascii_lower(int c) {
    return c >= 'a' && c <= 'z';
}

constexpr bool is_ascii_alpha(int c) {
    return is_ascii_upper(c) || is_ascii_lower(c);
}

constexpr bool is_ascii_alnum(int c) {
    return is_ascii_alpha(c) || is_ascii_digit(c);
}

constexpr bool is_ascii_xdigit(int c) {
    return is_ascii_digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

constexpr bool is_ascii_cntrl(int c) {
    return (c >= 0 && c < 0x20) || c == 0x7f;
}

constexpr bool is_ascii_punct(int c) {
    return is_ascii_printable(c) && !is_ascii_alnum(c) && c != ' ';
}

constexpr bool is_ascii_graph(int c) {
    return c >= 0x21 && c <= 0x7e;
}

}
