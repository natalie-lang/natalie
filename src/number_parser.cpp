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

        TM::Vector<Token> scan() {
            TM::Vector<Token> res {};
            while (true) {
                if (*m_curr == '\0') {
                    res.push({ TokenType::End, m_curr, 0 });
                    break;
                } else if (is_numeric(*m_curr)) {
                    size_t size = 1;
                    while (is_numeric(m_curr[size]))
                        size++;
                    res.push({ TokenType::Number, m_curr, size });
                    m_curr += size;
                } else if (*m_curr == '.') {
                    res.push({ TokenType::Period, m_curr, 1 });
                    m_curr++;
                } else {
                    res.push({ TokenType::Invalid, m_curr, 1 });
                    m_curr++;
                }
            }
            return res;
        }

    private:
        const char *m_curr { nullptr };
    };
}

NumberParser::NumberParser(const StringObject *str)
    : m_str { str->string() } { }

FloatObject *NumberParser::string_to_f(const StringObject *str) {
    NumberParser parser { str };
    const auto tokens = Tokenizer(parser.m_str).scan();
    switch (tokens[0].type) {
    case TokenType::Number: {
        if (tokens[1].type == TokenType::Period && tokens[2].type == TokenType::Number) {
            TM::String value { tokens[0].start, tokens[0].size + tokens[1].size + tokens[2].size };
            auto result = strtod(value.c_str(), nullptr);
            return new FloatObject { result };
        } else {
            TM::String value { tokens[0].start, tokens[0].size };
            auto result = strtod(value.c_str(), nullptr);
            return new FloatObject { result };
        }
    }
    case TokenType::Period:
        if (tokens[1].type == TokenType::Number) {
            TM::String value { tokens[0].start, tokens[0].size + tokens[1].size };
            auto result = strtod(value.c_str(), nullptr);
            return new FloatObject { result };
        } else {
            return new FloatObject { 0.0 };
        }
    case TokenType::Invalid:
    case TokenType::End:
        return new FloatObject { 0.0 };
    }

    NAT_UNREACHABLE();
}

}
