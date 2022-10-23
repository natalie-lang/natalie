#pragma once

#include "natalie.hpp"
#include "natalie/array_packer/tokenizer.hpp"

namespace Natalie {

class StringUnpacker : public Cell {
    using Tokenizer = ArrayPacker::Tokenizer;
    using Token = ArrayPacker::Token;

public:
    StringUnpacker(const StringObject *source, String directives)
        : m_source { source }
        , m_directives { Tokenizer { directives }.tokenize() } { }

    ~StringUnpacker() { delete m_directives; }

    ArrayObject *unpack(Env *env) {
        for (auto token : *m_directives) {
            if (token.error)
                env->raise("ArgumentError", *token.error);

            switch (token.directive) {
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
    void unpack_i() {
        if (at_end()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(Value::integer(*(int *)pointer()));
        m_index += sizeof(int);
    }

    void unpack_J() {
        if (at_end()) {
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

        auto str = new StringObject { out, EncodingObject::get(Encoding::ASCII_8BIT) };
        m_unpacked->push(str);
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

        auto str = new StringObject { out, EncodingObject::get(Encoding::ASCII_8BIT) };
        m_unpacked->push(str);
    }

    void unpack_P(Token &token) {
        if (at_end()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(new StringObject(*(const char **)pointer(), token.count));
        m_index += sizeof(uintptr_t);
    }

    void unpack_p() {
        if (at_end()) {
            m_unpacked->push(NilObject::the());
            return;
        }

        m_unpacked->push(new StringObject(*(const char **)pointer()));
        m_index += sizeof(uintptr_t);
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

    const char *pointer() { return m_source->c_str() + m_index; }

    bool at_end() { return m_index >= m_source->length(); }

    const StringObject *m_source;
    TM::Vector<Token> *m_directives;
    ArrayObject *m_unpacked { new ArrayObject };
    size_t m_index { 0 };
};

}
