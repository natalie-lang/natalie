#include "natalie/string_unpacker.hpp"
#include <array>

namespace Natalie {

namespace {

    // return a value between 0-63
    std::pair<int, bool> base64_decode_character(unsigned char c) {
        int value;
        if (c >= 'A' && c <= 'Z')
            value = c - 'A'; // A-Z is first
        else if (c >= 'a' && c <= 'z')
            value = c - 'a' + 26; // a-z comes after 26 A-Z characters
        else if (c >= '0' && c <= '9')
            value = c - '0' + 26 + 26; // 0-9 comes after 26 A-Z and 26 a-z characters
        else if (c == '+')
            value = 62; // next-to-last valid character
        else if (c == '/')
            value = 63; // last valid character
        else if (c == '=')
            value = 0; // padding character should be ignored elsewhere
        else
            return { 0, false }; // not valid, so return false
        return { value, true };
    }

    std::array<unsigned char, 3> base64_decode_sextet(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
        // each sextet (4 characters) represents 24 bits (each character is 6 bits)
        uint32_t triplet = (base64_decode_character(a).first << 3 * 6)
            + (base64_decode_character(b).first << 2 * 6)
            + (base64_decode_character(c).first << 1 * 6)
            + (base64_decode_character(d).first << 0 * 6);

        // 24 bits can represent 3 8-bit bytes
        unsigned char out_a = (triplet >> 2 * 8) & 0xFF;
        unsigned char out_b = (triplet >> 1 * 8) & 0xFF;
        unsigned char out_c = (triplet >> 0 * 8) & 0xFF;

        return { out_a, out_b, out_c };
    }
}

ArrayObject *StringUnpacker::unpack(Env *env) {
    for (auto token : *m_directives) {
        unpack_token(env, token);
    }
    return unpacked_array();
}

Value StringUnpacker::unpack1(Env *env) {
    for (auto token : *m_directives) {
        unpack_token(env, token);
        if (m_unpacked_value)
            return m_unpacked_value;
    }
    return NilObject::the();
}

void StringUnpacker::unpack_token(Env *env, Token &token) {
    if (token.error)
        env->raise("ArgumentError", *token.error);

    switch (token.directive) {
    case 'A':
        unpack_A(token);
        break;
    case 'a':
        unpack_a(token);
        break;
    case 'B':
        unpack_B(token);
        break;
    case 'b':
        unpack_b(token);
        break;
    case 'C':
        unpack_C(token);
        break;
    case 'c':
        unpack_c(token);
        break;
    case 'D':
    case 'd':
        unpack_float<double>(token, system_is_little_endian());
        break;
    case 'E':
        unpack_float<double>(token, true);
        break;
    case 'e':
        unpack_float<float>(token, true);
        break;
    case 'F':
    case 'f':
        unpack_float<float>(token, system_is_little_endian());
        break;
    case 'G':
        unpack_float<double>(token, false);
        break;
    case 'g':
        unpack_float<float>(token, false);
        break;
    case 'H':
        unpack_H(token);
        break;
    case 'h':
        unpack_h(token);
        break;
    case 'I':
        unpack_int<unsigned int>(token);
        break;
    case 'i':
        unpack_int<signed int>(token);
        break;
    case 'J':
        unpack_int<uintptr_t>(token);
        break;
    case 'j':
        unpack_int<intptr_t>(token);
        break;
    case 'L':
        if (token.native_size) {
            unpack_int<unsigned long>(token);
        } else {
            unpack_int<uint32_t>(token);
        }
        break;
    case 'l':
        if (token.native_size) {
            unpack_int<signed long>(token);
        } else {
            unpack_int<int32_t>(token);
        }
        break;
    case 'M':
        unpack_M(env, token);
        break;
    case 'm':
        unpack_m(env, token);
        break;
    case 'N':
        unpack_int<uint32_t>(token, false);
        break;
    case 'n':
        unpack_int<uint16_t>(token, false);
        break;
    case 'P':
        unpack_P(token);
        break;
    case 'p':
        unpack_p();
        break;
    case 'Q':
        unpack_int<uint64_t>(token);
        break;
    case 'q':
        unpack_int<int64_t>(token);
        break;
    case 'S':
        unpack_int<uint16_t>(token);
        break;
    case 's':
        unpack_int<int16_t>(token);
        break;
    case 'U':
        unpack_U(env, token);
        break;
    case 'u':
        unpack_u(token);
        break;
    case 'V':
        unpack_int<uint32_t>(token, true);
        break;
    case 'v':
        unpack_int<uint16_t>(token, true);
        break;
    case 'w':
        unpack_w(env, token);
        break;
    case 'X':
        unpack_X(env, token);
        break;
    case 'x':
        unpack_x(env, token);
        break;
    case 'Z':
        unpack_Z(token);
        break;
    case '@':
        unpack_at(env, token);
        break;
    default:
        env->raise("ArgumentError", "{} is not supported", token.directive);
    }
}

void StringUnpacker::unpack_A(Token &token) {
    if (at_end()) {
        append(new StringObject("", Encoding::ASCII_8BIT));
        return;
    }

    auto out = String();

    unpack_bytes(token, [&](unsigned char c) {
        out.append_char(c);
        return true;
    });

    // remove trailing null and space
    if (!out.is_empty()) {
        auto c = out.last_char();
        while (c == '\0' || c == ' ') {
            out.truncate(out.size() - 1);
            if (out.is_empty())
                break;
            c = out.last_char();
        }
    }

    append(new StringObject(out, Encoding::ASCII_8BIT));
}

void StringUnpacker::unpack_a(Token &token) {
    if (at_end()) {
        append(new StringObject("", Encoding::ASCII_8BIT));
        return;
    }

    auto out = String();

    unpack_bytes(token, [&](unsigned char c) {
        out.append_char(c);
        return true;
    });

    append(new StringObject(out, Encoding::ASCII_8BIT));
}

void StringUnpacker::unpack_B(Token &token) {
    auto out = String();

    unpack_bits(token, [&](unsigned char c) {
        if ((c & 0b10'000'000) == 0b10'000'000)
            out.append_char('1');
        else
            out.append_char('0');
        return c << 1;
    });

    append(new StringObject { out, Encoding::US_ASCII });
}

void StringUnpacker::unpack_C(Token &token) {
    nat_int_t consumed = 0;
    if (token.count == -1) token.count = 1;
    while (token.star ? !at_end() : consumed < token.count) {
        if (at_end()) {
            append(NilObject::the());
        } else {
            unsigned char c = pointer()[0];
            append(Value::integer((unsigned int)c));
            m_index++;
        }
        consumed++;
    }
}

void StringUnpacker::unpack_c(Token &token) {
    nat_int_t consumed = 0;
    if (token.count == -1) token.count = 1;
    while (token.star ? !at_end() : consumed < token.count) {
        if (at_end()) {
            append(NilObject::the());
        } else {
            signed char c = pointer()[0];
            append(Value::integer((signed int)c));
            m_index++;
        }
        consumed++;
    }
}

void StringUnpacker::unpack_b(Token &token) {
    auto out = String();

    unpack_bits(token, [&](unsigned char c) {
        if ((c & 0b1) == 0b1)
            out.append_char('1');
        else
            out.append_char('0');
        return c >> 1;
    });

    append(new StringObject { out, Encoding::US_ASCII });
}

void StringUnpacker::unpack_H(Token &token) {
    auto out = String();

    unpack_nibbles(token, [&](unsigned char c, int count) {
        out.append_sprintf("%x", c >> 4);
        if (count == 2)
            out.append_sprintf("%x", c & 0x0F);
    });

    append(new StringObject { out, Encoding::US_ASCII });
}

void StringUnpacker::unpack_h(Token &token) {
    auto out = String();

    unpack_nibbles(token, [&](unsigned char c, int count) {
        out.append_sprintf("%x", c & 0x0F);
        if (count == 2)
            out.append_sprintf("%x", c >> 4);
    });

    append(new StringObject { out, Encoding::US_ASCII });
}

void StringUnpacker::unpack_M(Env *env, Token &token) {
    auto out = String();

    auto next = [&]() -> unsigned char {
        if (at_end())
            return 0;

        unsigned char c = pointer()[0];
        m_index++;

        return c;
    };

    auto peek = [&]() -> unsigned char {
        if (m_index >= m_source->length())
            return 0;

        return pointer()[0];
    };

    while (!at_end()) {
        auto c = next();

        if (c == '=') {
            if (at_end()) {
                out.append_char('=');
                continue;
            }

            c = next();

            if (c == '\n') {
                void(); // skip it
            } else if (isxdigit(c) && isxdigit(peek())) {
                auto value = hex_char_to_decimal_value(c);
                value *= 16;
                value += hex_char_to_decimal_value(next());
                out.append_char((unsigned char)value);
            } else {
                out.append_char('=');
                out.append_char(c);
            }
            continue;
        }

        out.append_char(c);
    }

    append(new StringObject(out, Encoding::ASCII_8BIT));
}

void StringUnpacker::unpack_m(Env *env, Token &token) {
    auto out = String();

    auto next = [&]() -> unsigned char {
        unsigned char c;

        do {
            if (at_end())
                return 0;

            c = pointer()[0];

            auto valid_character = base64_decode_character(c).second;
            if (!valid_character) {
                if (token.count == 0)
                    // we only raise an error if the directive is m0
                    env->raise("ArgumentError", "invalid base64");
                else
                    // otherwise, we pretend its a newline (skip it)
                    c = '\n';
            }

            m_index++;
        } while (c == '\n');

        return c;
    };

    // loop over every 4 bytes (a sextet)
    // and convert it into 3 decoded characters
    while (!at_end()) {
        auto a = next();
        auto b = next();
        auto c = next();
        auto d = next();

        if (a == 0)
            break;

        auto chars = base64_decode_sextet(a, b, c, d);

        out.append_char(chars[0]);

        if (!c || c == '=')
            // ignore end-of-input and padding
            break;

        out.append_char(chars[1]);

        if (!d || d == '=')
            // ignore end-of-input and padding
            break;

        out.append_char(chars[2]);
    }

    if (!at_end()) {
        // extra unprocessed characters
        if (token.count == 0)
            // we only raise an error if the directive is m0
            env->raise("ArgumentError", "invalid base64");
    }

    append(new StringObject(out, Encoding::ASCII_8BIT));
}

void StringUnpacker::unpack_P(Token &token) {
    if (m_index + sizeof(uintptr_t) > m_source->length()) {
        append(NilObject::the());
        return;
    }

    const char *p = *(const char **)pointer();
    const size_t size = std::min(static_cast<size_t>(token.count), strlen(p));
    append(new StringObject(p, size));
    m_index += sizeof(uintptr_t);
}

void StringUnpacker::unpack_p() {
    if (m_index + sizeof(uintptr_t) > m_source->length()) {
        append(NilObject::the());
        return;
    }

    append(new StringObject(*(const char **)pointer()));
    m_index += sizeof(uintptr_t);
}

void StringUnpacker::unpack_U(Env *env, Token &token) {
    if (token.count == -1) token.count = 1;
    nat_int_t consumed = 0;
    while (!at_end() && (token.star || consumed < token.count)) {
        auto pair = m_source->next_char_result(&m_index);
        if (!pair.first)
            env->raise("ArgumentError", "malformed UTF-8 character");
        auto value = m_source->encoding()->decode_codepoint(pair.second);
        append(Value::integer(value));
        consumed++;
    }
}

void StringUnpacker::unpack_u(Token &token) {
    auto out = String();

    ssize_t count = 0;

    while (!at_end()) {
        // count is the first byte and can represent a line up to 45 decoded chars
        if (count == 0) {
            auto length_char = next_char();
            if (length_char >= 32 && length_char <= 95)
                count = length_char - 32;
            else
                // invalid length char, treat as zero
                break;
            if (count == 0)
                break;
        }

        // each encoded byte represents 6 bits of information, 24 bits in total
        auto a = next_char();
        auto b = next_char();
        auto c = next_char();
        auto d = next_char();

        // skip newlines in between 4-byte groups
        while (peek_char() == '\n')
            next_char();

        // ensure no number goes negative
        a = a < ' ' ? ' ' : a;
        b = b < ' ' ? ' ' : b;
        c = c < ' ' ? ' ' : c;
        d = d < ' ' ? ' ' : d;

        // The 4 groups of 6 bits get rearranged into 3 groups of 8 bits,
        // like this:
        //               |   a    |   b    |   c    |   d    |
        //     encoded:  |  '0'   |  'V'   |  '%'   |  'T'   |
        // subtract 32:  |  -32   |  -32   |  -32   |  -32   |
        //      mod 64:  |  %64   |  %64   |  %64   |  %64   |
        //        bits:  | 010000 | 110110 | 000101 | 110100 |
        //   regrouped:  |  01000011 | 01100001 |  01110100  |
        //       bytes:  |     67    |    97    |     116    |
        //       chars:  |    'C'    |   'a'    |     't'    |
        unsigned char out_a = (((a - 32) % 64) << 2) | (((b - 32) % 64) >> 4);
        unsigned char out_b = (((b - 32) % 64) << 4) | (((c - 32) % 64) >> 2);
        unsigned char out_c = (((c - 32) % 64) << 6) | (((d - 32) % 64) >> 0);

        out.append_char(out_a);
        if (--count == 0)
            continue;

        out.append_char(out_b);
        if (--count == 0)
            continue;

        out.append_char(out_c);
        if (--count == 0)
            continue;
    }

    append(new StringObject(out, Encoding::ASCII_8BIT));
}

void StringUnpacker::unpack_w(Env *env, Token &token) {
    const auto consumed = unpack_bytes(token, [&](unsigned char c) {
        // Bit shift operations on nat_int_t are faster than on BigInt, so keep
        // using native integers as long as possible and group the operations
        // performed on BigInt.
        constexpr auto max_fixnum_before_shift = NAT_MAX_FIXNUM >> 7;
        Integer result = 0;
        nat_int_t temp_result = 0;
        int shift = 0;
        bool keep_going = true;

        while (keep_going) {
            if (temp_result >= max_fixnum_before_shift) {
                result = (result << shift) | temp_result;
                temp_result = 0;
                shift = 0;
            }
            temp_result = (temp_result << 7) | (c & 0x7f);
            shift += 7;
            if (c & 0x80) {
                c = next_char();
            } else {
                keep_going = false;
            }
        }
        if (shift > 0) {
            result = (result << shift) | temp_result;
        }
        append(new IntegerObject(std::move(result)));

        return !at_end();
    });

    if (token.count > 0 && (ssize_t)consumed < token.count)
        m_index += (token.count - consumed);
}

void StringUnpacker::unpack_X(Env *env, Token &token) {
    nat_int_t new_position;
    if (token.star) {
        new_position = m_index - (m_source->length() - m_index);
    } else {
        if (token.count < 0) token.count = 1;
        new_position = m_index - token.count;
    }
    if (new_position < 0) {
        env->raise("ArgumentError", "movement cannot put pointer to less than zero");
    } else {
        m_index = (size_t)new_position;
    }
}

void StringUnpacker::unpack_x(Env *env, Token &token) {
    size_t new_position;
    if (token.star) {
        new_position = m_source->length();
    } else {
        if (token.count < 0) token.count = 1;
        new_position = m_index + token.count;
    }
    if (new_position > m_source->length()) {
        env->raise("ArgumentError", "movement cannot put pointer past the end");
    } else {
        m_index = new_position;
    }
}

void StringUnpacker::unpack_Z(Token &token) {
    if (at_end()) {
        append(new StringObject("", Encoding::ASCII_8BIT));
        return;
    }

    auto out = String();

    auto consumed = unpack_bytes(token, [&](unsigned char c) {
        if (c == '\0')
            return false;
        out.append_char(c);
        return true;
    });

    if (token.count > 0 && (ssize_t)consumed < token.count)
        m_index += (token.count - consumed);

    append(new StringObject(out, Encoding::ASCII_8BIT));
}

void StringUnpacker::unpack_at(Env *env, Token &token) {
    if (token.star) return; // star does nothing
    if (token.count < 0) {
        m_index = 0;
    } else {
        size_t new_position = std::abs(token.count);
        if (new_position > m_source->length()) {
            env->raise("ArgumentError", "count exceeds size of string");
        } else {
            m_index = new_position;
        }
    }
}

}
