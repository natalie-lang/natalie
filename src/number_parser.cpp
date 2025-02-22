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
            if (current().type == TokenType::Whitespace)
                advance();
            return parse_decimal_sign();
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

        TM::Optional<double> parse_decimal_sign() {
            if (current().type == TokenType::Sign && (peek().type == TokenType::Number || peek().type == TokenType::Period)) {
                append();
                advance();
            }
            return parse_decimal();
        }

        TM::Optional<double> parse_decimal() {
            if (current().type == TokenType::Number) {
                parse_number_sequence();
            } else if (current().type == TokenType::Period && peek().type == TokenType::Number) {
                append_char('0');
            } else {
                return {};
            }
            return parse_fraction();
        }

        TM::Optional<double> parse_fraction() {
            if (current().type == TokenType::Period) {
                if (peek().type == TokenType::Number) {
                    append();
                    advance();
                    parse_number_sequence();
                } else {
                    advance();
                }
            }
            return parse_scientific_e();
        }

        TM::Optional<double> parse_scientific_e() {
            if (current().type == TokenType::ScientificE) {
                advance();
                if (current().type == TokenType::Number) {
                    append_char('e');
                } else if (current().type == TokenType::Sign && peek().type == TokenType::Number) {
                    append_char('e');
                    append();
                    advance();
                }
                if (current().type == TokenType::Number)
                    parse_number_sequence();
            }
            return strtod(m_result.c_str(), nullptr);
        }
    };
}

FloatObject *NumberParser::string_to_f(TM::NonNullPtr<const StringObject> str) {
    FloatParser float_parser { str->string() };
    return new FloatObject { float_parser().value_or(0.0) };
}

}
