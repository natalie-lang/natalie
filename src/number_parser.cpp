#include "natalie.hpp"

namespace Natalie {

namespace {
    enum class TokenType {
        Number,
        Period,
        Sign,
        Whitespace,
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

    bool is_whitespace(const char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
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
            } else if (is_whitespace(*m_curr)) {
                size_t size = 1;
                while (is_whitespace(m_curr[size]))
                    size++;
                return make_token(TokenType::Whitespace, size);
            } else if (*m_curr == '.') {
                return make_token(TokenType::Period, 1);
            } else if (*m_curr == '+' || *m_curr == '-') {
                return make_token(TokenType::Sign, 1);
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

    class FloatParser {
    public:
        FloatParser(const TM::String &str)
            : m_tokenizer { Tokenizer { str } } { }

        double operator()() {
            const auto token = scan();
            switch (token.type) {
            case TokenType::Whitespace:
                // Skip and continue
                return operator()();
            case TokenType::Sign:
                parse_sign(token);
                break;
            case TokenType::Number:
                parse_decimal(token);
                break;
            case TokenType::Period:
                append_char('0');
                parse_fractional(token);
                break;
            case TokenType::Invalid:
            case TokenType::End:
                return 0.0;
            }

            return strtod(m_result.c_str(), nullptr);
        }

    private:
        Tokenizer m_tokenizer;
        TM::String m_result {};

        Token scan() { return m_tokenizer.scan(); }
        void append_char(const char c) { m_result.append_char(c); }
        void append(const Token &token) { m_result.append(token.start, token.size); }

        void parse_sign(const Token &token) {
            const auto next_token = scan();
            if (next_token.type == TokenType::Number) {
                append(token);
                parse_decimal(next_token);
            } else if (next_token.type == TokenType::Period) {
                append(token);
                append_char('0');
                parse_fractional(next_token);
            }
        }

        void parse_decimal(const Token &token) {
            append(token);
            const auto next_token = scan();
            if (next_token.type == TokenType::Period)
                parse_fractional(next_token);
        }

        void parse_fractional(const Token &token) {
            const auto next_token = scan();
            if (next_token.type == TokenType::Number) {
                append(token);
                append(next_token);
            }
        }
    };
}

FloatObject *NumberParser::string_to_f(const StringObject *str) {
    FloatParser float_parser { str->string() };
    return new FloatObject { float_parser() };
}

}
