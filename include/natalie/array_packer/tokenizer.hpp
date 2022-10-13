#pragma once

#include "tm/optional.hpp"
#include "tm/string.hpp"
#include "tm/vector.hpp"

namespace Natalie {

using namespace TM;

namespace ArrayPacker {
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
        Endianness endianness { Endianness::Native };
        Optional<String> error {};
    };

    class Tokenizer {
    public:
        Tokenizer(String directives)
            : m_directives { directives } { }

        Vector<Token> *tokenize() {
            auto tokens = new Vector<Token> {};
            for (Token token = next_token(); token.directive; token = next_token()) {
                tokens->push(token);
                if (token.error)
                    break;
            }
            return tokens;
        }

    private:
        Token next_token() {
            auto d = current_char();
            if (!d) return {};

            auto token = Token { d };

            auto modifier = next_char();

            if (modifier == '>') {
                token.endianness = Endianness::Big;
                modifier = next_char();
            }

            if (modifier == '<') {
                token.endianness = Endianness::Little;
                modifier = next_char();
            }

            if (modifier == '_' || modifier == '!') {
                switch (token.directive) {
                case 'i':
                case 'I':
                case 'j':
                case 'J':
                case 'l':
                case 'L':
                case 'q':
                case 'Q':
                case 's':
                case 'S':
                    token.native_size = true;
                    break;
                default:
                    token.error = String::format("'{}' allowed only after types sSiIlLqQjJ", modifier);
                }
                modifier = next_char();
            }

            if (modifier == '>') {
                token.endianness = Endianness::Big;
                modifier = next_char();
            }

            if (modifier == '<') {
                token.endianness = Endianness::Little;
                modifier = next_char();
            }

            if (isdigit(modifier)) {
                token.count = (int)modifier - '0';
                modifier = next_char();
                while (isdigit(modifier)) {
                    token.count *= 10;
                    token.count += ((int)modifier - '0');
                    modifier = next_char();
                }
            }

            if (modifier == '*') {
                next_char();
                token.star = true;
            }

            // printf("token { '%c', %d, %d, %d }\n", token.directive, token.count, (int)token.star, (int)token.native_size);

            return token;
        }

        signed char next_char() {
            m_index++;
            return current_char();
        }

        signed char current_char() {
            if (m_index >= m_directives.length())
                return 0;
            signed char c = m_directives[m_index];
            while (isspace(c) || c == '\0') {
                if (m_index + 1 >= m_directives.length())
                    return 0;
                c = m_directives[++m_index];
            }
            if (c == '#') {
                while (c != '\n') {
                    if (m_index + 1 >= m_directives.length())
                        return 0;
                    c = m_directives[++m_index];
                }
                if (m_index + 1 >= m_directives.length())
                    return 0;
                c = m_directives[++m_index];
            }
            return c;
        }

        String m_directives;
        size_t m_index { 0 };
    };

}

}
