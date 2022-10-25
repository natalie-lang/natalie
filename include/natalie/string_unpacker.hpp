#pragma once

#include "natalie.hpp"
#include "natalie/array_packer/tokenizer.hpp"

namespace Natalie {

class StringUnpacker : public Cell {
    using Tokenizer = ArrayPacker::Tokenizer;
    using Token = ArrayPacker::Token;

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
            case 'i':
                unpack_i();
                break;
            case 'J':
                unpack_J();
                break;
            case 'M':
                unpack_M(env, token);
                break;
            case 'm':
                unpack_m(env, token);
                break;
            case 'P':
                unpack_P(token);
                break;
            case 'p':
                unpack_p();
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
            m_unpacked->push(StringObject::binary());
            return;
        }

        auto out = String();

        unpack_with_loop(token, [&]() {
            out.append_char(next_char());
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

        m_unpacked->push(StringObject::binary(out));
    }

    void unpack_a(Token &token) {
        if (at_end()) {
            m_unpacked->push(StringObject::binary());
            return;
        }

        auto out = String();

        unpack_with_loop(token, [&]() {
            out.append_char(next_char());
            return true;
        });

        m_unpacked->push(StringObject::binary(out));
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

        m_unpacked->push(new StringObject { out, EncodingObject::get(Encoding::US_ASCII) });
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

        m_unpacked->push(new StringObject { out, EncodingObject::get(Encoding::US_ASCII) });
    }

    void unpack_i() {
        if (m_index + sizeof(int) > m_source->length()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(Value::integer(*(int *)pointer()));
        m_index += sizeof(int);
    }

    void unpack_J() {
        if (m_index + sizeof(uintptr_t) > m_source->length()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(Value::integer(*(uintptr_t *)pointer()));
        m_index += sizeof(uintptr_t);
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

        m_unpacked->push(StringObject::binary(out));
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

        m_unpacked->push(StringObject::binary(out));
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

    void unpack_Z(Token &token) {
        if (at_end()) {
            m_unpacked->push(StringObject::binary());
            return;
        }

        auto out = String();

        auto consumed = unpack_with_loop(token, [&]() {
            auto c = next_char();
            if (c == '\0')
                return false;
            out.append_char(c);
            return true;
        });

        if (token.count > 0 && (ssize_t)consumed < token.count)
            m_index += (token.count - consumed);

        m_unpacked->push(StringObject::binary(out));
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
    size_t unpack_with_loop(Token &token, Fn handler) {
        size_t start = m_index;
        if (token.star) {
            while (!at_end()) {
                auto keep_going = handler();
                if (!keep_going)
                    break;
            }
        } else if (token.count != -1) {
            for (int i = 0; i < token.count; ++i) {
                if (at_end())
                    break;
                auto keep_going = handler();
                if (!keep_going)
                    break;
            }
        } else if (!at_end()) {
            handler();
        }
        assert(start <= m_index); // going backwards is unexpected
        return m_index - start;
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

    const StringObject *m_source;
    TM::Vector<Token> *m_directives;
    ArrayObject *m_unpacked { new ArrayObject };
    size_t m_index { 0 };
};

}
