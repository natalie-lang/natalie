#include "natalie.hpp"

namespace Natalie {

namespace {
    enum class TokenType {
        Number,
        Period,
        Invalid,
        End,
    };

    struct Token {
        TokenType type;
        const char *start;
        size_t size;
    };

    bool is_numeric(const char c) {
        return c >= '0' && c <= '9';
    }

    class Tokenizer {
    public:
        Tokenizer(const TM::String &str)
            : m_curr { str.c_str() } { }

        Token scan() {
            if (*m_curr == '\0') {
                return make_token(TokenType::End, 0);
            } else if (is_numeric(*m_curr)) {
                size_t size = 1;
                while (is_numeric(m_curr[size]))
                    size++;
                return make_token(TokenType::Number, size);
            } else if (*m_curr == '.') {
                return make_token(TokenType::Period, 1);
            } else {
                return make_token(TokenType::Invalid, 1);
            }
        }

    private:
        const char *m_curr { nullptr };

        Token make_token(TokenType type, size_t size) {
            Token token { type, m_curr, size };
            m_curr += size;
            return token;
        }
    };
}

NumberParser::NumberParser(const StringObject *str)
    : m_str { str->string() } { }

FloatObject *NumberParser::string_to_f(const StringObject *str) {
    NumberParser parser { str };
    Tokenizer tokenizer { parser.m_str };

    const auto token = tokenizer.scan();
    switch (token.type) {
    case TokenType::Number: {
        auto next_token = tokenizer.scan();
        if (next_token.type == TokenType::Period) {
            next_token = tokenizer.scan();
            if (next_token.type == TokenType::Number) {
                // "x.y"
                TM::String value { token.start, token.size };
                value.append_char('.');
                value.append(next_token.start, next_token.size);
                auto result = strtod(value.c_str(), nullptr);
                return new FloatObject { result };
            }
        }
        // "x" or "x."
        TM::String value { token.start, token.size };
        auto result = strtod(value.c_str(), nullptr);
        return new FloatObject { result };
    }
    case TokenType::Period: {
        auto next_token = tokenizer.scan();
        if (next_token.type == TokenType::Number) {
            // ".y"
            TM::String value { "0." };
            value.append(next_token.start, next_token.size);
            auto result = strtod(value.c_str(), nullptr);
            return new FloatObject { result };
        }
        return new FloatObject { 0.0 };
    }
    case TokenType::Invalid:
    case TokenType::End:
        return new FloatObject { 0.0 };
    }

    NAT_UNREACHABLE();
}

}
