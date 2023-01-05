#include "natalie/array_packer/tokenizer.hpp"

namespace Natalie {

using namespace TM;

namespace ArrayPacker {

    namespace {

        void apply_modifier_error_if_not_allowed(Token &token, signed char modifier) {
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
                return;
            default:
                token.error = String::format("'{}' allowed only after types sSiIlLqQjJ", modifier);
            }
        }

        bool apply_endianness_modifier(Token &token, signed char modifier) {
            switch (modifier) {
            case '>':
                apply_modifier_error_if_not_allowed(token, modifier);
                token.endianness = Endianness::Big;
                return true;
            case '<':
                apply_modifier_error_if_not_allowed(token, modifier);
                token.endianness = Endianness::Little;
                return true;
            default:
                return false;
            }
        }

    }

    Token Tokenizer::next_token() {
        auto directive = current_char();
        if (!directive) return {};

        auto token = Token { directive };

        auto modifier = next_char();

        if (apply_endianness_modifier(token, modifier))
            modifier = next_char();

        if (modifier == '_' || modifier == '!') {
            apply_modifier_error_if_not_allowed(token, modifier);
            token.native_size = true;
            modifier = next_char();
        }

        if (apply_endianness_modifier(token, modifier))
            modifier = next_char();

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

        // printf("token { directive='%c', count=%d, star=%d, native_size=%d, endianness=%d }\n", token.directive, token.count, (int)token.star, (int)token.native_size, (int)token.endianness);

        return token;
    }

    signed char Tokenizer::next_char() {
        if (m_index >= m_directives.length())
            return 0;
        m_index++;
        return current_char();
    }

    signed char Tokenizer::current_char() {
        signed char c = char_at_index(m_index);

        while (m_index < m_directives.size() && (isspace(c) || c == '\0'))
            c = char_at_index(++m_index);

        if (c == '#') {
            while (c && c != '\n')
                c = char_at_index(++m_index);
            if (c == '\n')
                c = char_at_index(++m_index);
        }

        return c;
    }

    signed char Tokenizer::char_at_index(size_t index) const {
        if (index >= m_directives.length())
            return 0;
        return m_directives[index];
    }

}

}
