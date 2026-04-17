#pragma once

#include "natalie/ctype.hpp"
#include "tm/optional.hpp"
#include "tm/string.hpp"
#include "tm/vector.hpp"

namespace Natalie {

using namespace TM;

namespace ArrayPacker {
    // Escape non-printable bytes in a directive string the way MRI's
    // rb_str_quote_unprintable does, so error messages match what MRI produces.
    inline String quote_unprintable(const String &str) {
        String out;
        for (size_t i = 0; i < str.size(); i++) {
            unsigned char c = str[i];
            switch (c) {
            case '\0':
                out.append("\\0");
                break;
            case '\a':
                out.append("\\a");
                break;
            case '\b':
                out.append("\\b");
                break;
            case '\t':
                out.append("\\t");
                break;
            case '\n':
                out.append("\\n");
                break;
            case '\v':
                out.append("\\v");
                break;
            case '\f':
                out.append("\\f");
                break;
            case '\r':
                out.append("\\r");
                break;
            case '\x1b':
                out.append("\\e");
                break;
            case '\x7f':
                out.append("\\c?");
                break;
            default:
                if (c < 0x20) {
                    auto hex = String::hex(c, String::HexFormat::Uppercase);
                    while (hex.length() < 4)
                        hex.prepend_char('0');
                    out.append("\\u");
                    out.append(hex);
                } else if (c >= 0x80) {
                    auto hex = String::hex(c, String::HexFormat::Uppercase);
                    if (hex.length() < 2)
                        hex.prepend_char('0');
                    out.append("\\x");
                    out.append(hex);
                } else {
                    out.append_char(c);
                }
            }
        }
        return out;
    }

    enum class Endianness {
        Native,
        Little,
        Big,
    };

    struct Token {
        signed char directive { 0 };
        int count { -1 };
        bool star { false };
        bool native_size { false };
        bool unknown { false };
        Endianness endianness { Endianness::Native };
        Optional<String> error {};
    };

    class Tokenizer {
    public:
        Tokenizer(String directives)
            : m_directives { directives } { }

        Vector<Token> *tokenize() {
            auto tokens = new Vector<Token> {};
            while (true) {
                auto token = next_token();
                if (!token.directive && !token.error && !token.unknown)
                    break;
                tokens->push(token);
                if (token.error)
                    break;
            }
            return tokens;
        }

    private:
        Token next_token();
        signed char next_char();
        signed char current_char();
        signed char char_at_index(size_t index) const;

        String m_directives;
        size_t m_index { 0 };
    };

}

}
