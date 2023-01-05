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
        Token next_token();
        signed char next_char();
        signed char current_char();
        signed char char_at_index(size_t index) const;

        String m_directives;
        size_t m_index { 0 };
    };

}

}
