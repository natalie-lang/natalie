#pragma once

#include "natalie.hpp"
#include "natalie/array_packer/tokenizer.hpp"
#include <array>

namespace Natalie {

class StringUnpacker : public Cell {
    using Tokenizer = ArrayPacker::Tokenizer;
    using Token = ArrayPacker::Token;
    using Endianness = ArrayPacker::Endianness;

public:
    StringUnpacker(const StringObject *source, String directives, nat_int_t offset)
        : m_source { source }
        , m_directives { Tokenizer { directives }.tokenize() }
        , m_index { (size_t)std::max(offset, (nat_int_t)0) } { }

    ~StringUnpacker() { delete m_directives; }

    ArrayObject *unpack(Env *env) {
        for (auto token : *m_directives) {
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
            case 'Z':
                unpack_Z(token);
                break;
            default:
                env->raise("ArgumentError", "{} is not supported", token.directive);
            }
        }
        return m_unpacked;
    }

    virtual void visit_children(Visitor &visitor) override {
        visitor.visit(m_source);
        visitor.visit(m_unpacked);
    }

private:
    void unpack_A(Token &token) {
        if (at_end()) {
            m_unpacked->push(new StringObject("", Encoding::ASCII_8BIT));
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

        m_unpacked->push(new StringObject(out, Encoding::ASCII_8BIT));
    }

    void unpack_a(Token &token) {
        if (at_end()) {
            m_unpacked->push(new StringObject("", Encoding::ASCII_8BIT));
            return;
        }

        auto out = String();

        unpack_bytes(token, [&](unsigned char c) {
            out.append_char(c);
            return true;
        });

        m_unpacked->push(new StringObject(out, Encoding::ASCII_8BIT));
    }

    void unpack_B(Token &token) {
        auto out = String();

        unpack_bits(token, [&](unsigned char c) {
            if ((c & 0b10'000'000) == 0b10'000'000)
                out.append_char('1');
            else
                out.append_char('0');
            return c << 1;
        });

        m_unpacked->push(new StringObject { out, Encoding::US_ASCII });
    }

    void unpack_C(Token &token) {
        nat_int_t consumed = 0;
        if (token.count == -1) token.count = 1;
        while (token.star ? !at_end() : consumed < token.count) {
            if (at_end()) {
                m_unpacked->push(NilObject::the());
            } else {
                unsigned char c = pointer()[0];
                m_unpacked->push(Value::integer((unsigned int)c));
                m_index++;
            }
            consumed++;
        }
    }

    void unpack_c(Token &token) {
        nat_int_t consumed = 0;
        if (token.count == -1) token.count = 1;
        while (token.star ? !at_end() : consumed < token.count) {
            if (at_end()) {
                m_unpacked->push(NilObject::the());
            } else {
                signed char c = pointer()[0];
                m_unpacked->push(Value::integer((signed int)c));
                m_index++;
            }
            consumed++;
        }
    }

    void unpack_b(Token &token) {
        auto out = String();

        unpack_bits(token, [&](unsigned char c) {
            if ((c & 0b1) == 0b1)
                out.append_char('1');
            else
                out.append_char('0');
            return c >> 1;
        });

        m_unpacked->push(new StringObject { out, Encoding::US_ASCII });
    }

    template <typename T>
    void unpack_float(Token &token, bool little_endian) {
        nat_int_t consumed = 0;
        if (token.count == -1) token.count = 1;
        while (token.star ? !at_end() : consumed < token.count) {
            if ((m_index + sizeof(T)) > m_source->length()) {
                if (!token.star)
                    m_unpacked->push(NilObject::the());
                m_index++;
            } else {
                // NATFIXME: this method of fixing endianness may not be efficient
                // reverse a character buffer based on endianness
                auto out = String();
                for (size_t idx = 0; idx < sizeof(T); idx++) {
                    auto realidx = (little_endian) ? idx : (sizeof(T) - 1 - idx);
                    out.append_char(pointer()[realidx]);
                }
                m_index += sizeof(T);
                double double_val = double(*(T *)out.c_str());
                auto fltobj = new FloatObject { double_val };
                m_unpacked->push(fltobj);
            }
            consumed++;
        }
    }

    void unpack_H(Token &token) {
        auto out = String();

        unpack_nibbles(token, [&](unsigned char c, int count) {
            out.append_sprintf("%x", c >> 4);
            if (count == 2)
                out.append_sprintf("%x", c & 0x0F);
        });

        m_unpacked->push(new StringObject { out, Encoding::US_ASCII });
    }

    void unpack_h(Token &token) {
        auto out = String();

        unpack_nibbles(token, [&](unsigned char c, int count) {
            out.append_sprintf("%x", c & 0x0F);
            if (count == 2)
                out.append_sprintf("%x", c >> 4);
        });

        m_unpacked->push(new StringObject { out, Encoding::US_ASCII });
    }

    template <typename T>
    void unpack_int(Token &token) {
        bool little_endian = (token.endianness == ArrayPacker::Endianness::Little) || (token.endianness == Endianness::Native && system_is_little_endian());
        unpack_int<T>(token, little_endian);
    }

    template <typename T>
    void unpack_int(Token &token, bool little_endian) {
        nat_int_t consumed = 0;
        if (token.count == -1) token.count = 1;
        while (token.star ? !at_end() : consumed < token.count) {
            if ((m_index + sizeof(T)) > m_source->length()) {
                if (!token.star)
                    m_unpacked->push(NilObject::the());
                m_index++;
            } else {
                // NATFIXME: this method of fixing endianness may not be efficient
                // reverse a character buffer based on endianness
                auto out = String();
                for (size_t idx = 0; idx < sizeof(T); idx++) {
                    auto realidx = (little_endian) ? idx : (sizeof(T) - 1 - idx);
                    out.append_char(pointer()[realidx]);
                }
                m_index += sizeof(T);
                // Previosly had pushed Value::integer() values, but for large unsigned values
                // it produced incorrect results.
                auto bigint = BigInt(*(T *)out.c_str());
                m_unpacked->push(IntegerObject::create(Integer(bigint)));
            }
            consumed++;
        }
    }

    void unpack_M(Env *env, Token &token) {
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

        m_unpacked->push(new StringObject(out, Encoding::ASCII_8BIT));
    }

    void unpack_m(Env *env, Token &token) {
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

        m_unpacked->push(new StringObject(out, Encoding::ASCII_8BIT));
    }

    void unpack_P(Token &token) {
        if (m_index + sizeof(uintptr_t) > m_source->length()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(new StringObject(*(const char **)pointer(), token.count));
        m_index += sizeof(uintptr_t);
    }

    void unpack_p() {
        if (m_index + sizeof(uintptr_t) > m_source->length()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(new StringObject(*(const char **)pointer()));
        m_index += sizeof(uintptr_t);
    }

    void unpack_U(Env *env, Token &token) {
        if (token.count == -1) token.count = 1;
        nat_int_t consumed = 0;
        while (!at_end() && (token.star || consumed < token.count)) {
            auto pair = m_source->next_char_result(&m_index);
            if (!pair.first)
                env->raise("ArgumentError", "malformed UTF-8 character");
            auto value = m_source->encoding()->decode_codepoint(pair.second);
            m_unpacked->push(Value::integer(value));
            consumed++;
        }
    }

    void unpack_u(Token &token) {
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

        m_unpacked->push(new StringObject(out, Encoding::ASCII_8BIT));
    }

    void unpack_Z(Token &token) {
        if (at_end()) {
            m_unpacked->push(new StringObject("", Encoding::ASCII_8BIT));
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

        m_unpacked->push(new StringObject(out, Encoding::ASCII_8BIT));
    }

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

    template <typename Fn>
    size_t unpack_bytes(Token &token, Fn handler) {
        size_t start = m_index;
        if (token.star) {
            while (!at_end()) {
                auto keep_going = handler(next_char());
                if (!keep_going)
                    break;
            }
        } else if (token.count != -1) {
            for (int i = 0; i < token.count; ++i) {
                if (at_end())
                    break;
                auto keep_going = handler(next_char());
                if (!keep_going)
                    break;
            }
        } else if (!at_end()) {
            handler(next_char());
        }
        assert(start <= m_index); // going backwards is unexpected
        return m_index - start;
    }

    template <typename Fn>
    void unpack_nibbles(Token &token, Fn handler) {
        if (token.star) {
            while (!at_end())
                handler(next_char(), 2);
        } else if (token.count != 0) {
            auto count = std::max(1, token.count);
            while (count > 0 && !at_end()) {
                handler(next_char(), count >= 2 ? 2 : 1);
                count -= 2;
            }
        }
    }

    template <typename Fn>
    void unpack_bits(Token &token, Fn handler) {
        auto bits_remaining = token.count;
        if (bits_remaining < 0)
            bits_remaining = 1;
        if (token.star)
            bits_remaining = 8;

        while (bits_remaining > 0 && !at_end()) {
            auto c = next_char();

            for (int i = 0; i < std::min(8, bits_remaining); i++)
                c = handler(c);

            if (token.star)
                bits_remaining = 8;
            else if (bits_remaining >= 8)
                bits_remaining -= 8;
            else
                bits_remaining = 0;
        }
    }

    const char *pointer() { return m_source->c_str() + m_index; }

    unsigned char next_char() {
        if (at_end())
            return 0;
        return m_source->at(m_index++);
    }

    unsigned char peek_char() {
        if (at_end())
            return 0;
        return m_source->at(m_index);
    }

    bool at_end() { return m_index >= m_source->length(); }

    bool system_is_little_endian() {
        int i = 1;
        return *((char *)&i) == 1;
    }

    const StringObject *m_source;
    TM::Vector<Token> *m_directives;
    ArrayObject *m_unpacked { new ArrayObject };
    size_t m_index { 0 };
};
}
