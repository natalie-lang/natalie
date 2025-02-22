#include "natalie.hpp"

namespace Natalie {

namespace {
    enum class TokenType {
        Number,
        Period,
        Sign,
        ScientificE,
        Underscore,
        Whitespace,
        Invalid,
        End,
        NotYetScanned,
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
            } else if (*m_curr == 'e' || *m_curr == 'E') {
                return make_token(TokenType::ScientificE, 1);
            } else if (*m_curr == '_') {
                return make_token(TokenType::Underscore, 1);
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

        TM::Optional<double> operator()() {
            advance();
            switch (current().type) {
            case TokenType::Whitespace:
                // Skip and continue
                return operator()();
            case TokenType::Sign:
                parse_sign();
                break;
            case TokenType::Number:
                parse_decimal();
                break;
            case TokenType::Period:
                append_char('0');
                parse_fractional();
                break;
            case TokenType::ScientificE:
            case TokenType::Underscore:
            case TokenType::Invalid:
            case TokenType::End:
            case TokenType::NotYetScanned:
                return {};
            }

            return strtod(m_result.c_str(), nullptr);
        }

    private:
        Tokenizer m_tokenizer;
        Token m_current { TokenType::NotYetScanned, nullptr, 0 };
        Token m_next { TokenType::NotYetScanned, nullptr, 0 };
        TM::String m_result {};

        Token current() { return m_current; }

        Token peek() {
            assert(m_current.type != TokenType::NotYetScanned);
            if (m_next.type == TokenType::NotYetScanned)
                m_next = m_tokenizer.scan();
            return m_next;
        }

        void advance() {
            if (m_next.type == TokenType::NotYetScanned) {
                m_current = m_tokenizer.scan();
            } else {
                m_current = m_next;
                m_next = Token { TokenType::NotYetScanned, nullptr, 0 };
            }
        }

        void append_char(const char c) { m_result.append_char(c); }
        void append() { m_result.append(current().start, current().size); }

        void parse_number_sequence() {
            append();
            advance();
            if (current().type == TokenType::Underscore && peek().type == TokenType::Number) {
                advance();
                parse_number_sequence();
            }
        }

        void parse_sign() {
            if (peek().type == TokenType::Number) {
                append();
                advance();
                parse_decimal();
            } else if (peek().type == TokenType::Period) {
                append();
                append_char('0');
                advance();
                parse_fractional();
            }
        }

        void parse_decimal() {
            parse_number_sequence();
            if (current().type == TokenType::Period) {
                parse_fractional();
            } else if (current().type == TokenType::ScientificE) {
                parse_scientific_e();
            }
        }

        void parse_fractional() {
            if (peek().type == TokenType::Number) {
                append();
                advance();
                parse_number_sequence();
                if (current().type == TokenType::ScientificE) {
                    parse_scientific_e();
                }
                return;
            }
            if (peek().type == TokenType::ScientificE) {
                advance();
                parse_scientific_e();
            }
        }

        void parse_scientific_e() {
            if (peek().type == TokenType::Number) {
                append();
                advance();
                parse_number_sequence();
            } else if (peek().type == TokenType::Sign) {
                advance();
                if (peek().type == TokenType::Number) {
                    append_char('e');
                    append();
                    advance();
                    parse_number_sequence();
                }
            }
        }
    };
}

FloatObject *NumberParser::string_to_f(TM::NonNullPtr<const StringObject> str) {
    FloatParser float_parser { str->string() };
    return new FloatObject { float_parser().value_or(0.0) };
}

}
