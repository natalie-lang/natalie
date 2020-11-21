#include <string>

#include "natalie.hpp"

namespace Natalie {

struct Lexer : public gc {
    Lexer(const char *input)
        : m_input { input }
        , m_size { strlen(input) } {
        assert(m_input);
    }

    struct Token : public gc {
        enum class Type {
            Alias,
            And,
            Begin,
            BEGIN,
            Break,
            Case,
            Class,
            Comma,
            Constant,
            Def,
            Defined,
            Divide,
            Do,
            Else,
            Elsif,
            _ENCODING_,
            End,
            END,
            Ensure,
            Eof,
            Eol,
            Equal,
            EqualEqual,
            EqualEqualEqual,
            Fail,
            False,
            _FILE_,
            For,
            HashRocket,
            Identifier,
            If,
            In,
            Integer,
            Invalid,
            LBrace,
            LBracket,
            _LINE_,
            Loop,
            LParen,
            Minus,
            Module,
            Multiply,
            Next,
            Nil,
            Not,
            Or,
            Plus,
            RBrace,
            RBracket,
            Redo,
            Regexp,
            Rescue,
            Retry,
            Return,
            RParen,
            Self,
            Semicolon,
            String,
            Super,
            Symbol,
            SymbolKey,
            Then,
            True,
            Undef,
            Unless,
            UnterminatedRegexp,
            UnterminatedString,
            Until,
            When,
            While,
            Yield,
        };

        Token() { }

        Token(Type type)
            : m_type { type } { }

        Token(Type type, const char *literal)
            : m_type { type }
            , m_literal { literal } {
            assert(m_literal);
        }

        Token(Type type, nat_int_t integer)
            : m_type { type }
            , m_integer { integer } { }

        Type type() { return m_type; }
        const char *literal() { return m_literal; }
        nat_int_t integer() { return m_integer; }

        Value *to_ruby(Env *env) {
            switch (m_type) {
            case Type::_ENCODING_:
                return SymbolValue::intern(env, "__ENCODING__");
            case Type::_FILE_:
                return SymbolValue::intern(env, "__FILE__");
            case Type::_LINE_:
                return SymbolValue::intern(env, "__LINE__");
            case Type::Alias:
                return SymbolValue::intern(env, "alias");
            case Type::And:
                return SymbolValue::intern(env, "and");
            case Type::Begin:
                return SymbolValue::intern(env, "begin");
            case Type::BEGIN:
                return SymbolValue::intern(env, "BEGIN");
            case Type::Break:
                return SymbolValue::intern(env, "break");
            case Type::Case:
                return SymbolValue::intern(env, "case");
            case Type::Class:
                return SymbolValue::intern(env, "class");
            case Type::Comma:
                return SymbolValue::intern(env, ",");
            case Type::Constant:
                return new ArrayValue { env, { SymbolValue::intern(env, "constant"), SymbolValue::intern(env, m_literal) } };
            case Type::Def:
                return SymbolValue::intern(env, "def");
            case Type::Defined:
                return SymbolValue::intern(env, "defined?");
            case Type::Divide:
                return SymbolValue::intern(env, "/");
            case Type::Do:
                return SymbolValue::intern(env, "do");
            case Type::End:
                return SymbolValue::intern(env, "end");
            case Type::Else:
                return SymbolValue::intern(env, "else");
            case Type::Elsif:
                return SymbolValue::intern(env, "elsif");
            case Type::END:
                return SymbolValue::intern(env, "END");
            case Type::Ensure:
                return SymbolValue::intern(env, "ensure");
            case Type::Eol:
                return SymbolValue::intern(env, "\n");
            case Type::Eof:
                return env->nil_obj();
            case Type::Equal:
                return SymbolValue::intern(env, "=");
            case Type::EqualEqual:
                return SymbolValue::intern(env, "==");
            case Type::EqualEqualEqual:
                return SymbolValue::intern(env, "===");
            case Type::Fail:
                return SymbolValue::intern(env, "fail");
            case Type::False:
                return SymbolValue::intern(env, "false");
            case Type::For:
                return SymbolValue::intern(env, "for");
            case Type::HashRocket:
                return SymbolValue::intern(env, "=>");
            case Type::Identifier:
                return new ArrayValue { env, { SymbolValue::intern(env, "identifier"), SymbolValue::intern(env, m_literal) } };
            case Type::If:
                return SymbolValue::intern(env, "if");
            case Type::In:
                return SymbolValue::intern(env, "in");
            case Type::Integer:
                return new ArrayValue { env, { SymbolValue::intern(env, "integer"), new IntegerValue { env, m_integer } } };
            case Type::Invalid:
                env->raise("SyntaxError", "syntax error, unexpected '%s'", m_literal);
            case Type::LBrace:
                return SymbolValue::intern(env, "{");
            case Type::LBracket:
                return SymbolValue::intern(env, "[");
            case Type::LParen:
                return SymbolValue::intern(env, "(");
            case Type::Loop:
                return SymbolValue::intern(env, "loop");
            case Type::Minus:
                return SymbolValue::intern(env, "-");
            case Type::Module:
                return SymbolValue::intern(env, "module");
            case Type::Multiply:
                return SymbolValue::intern(env, "*");
            case Type::Next:
                return SymbolValue::intern(env, "next");
            case Type::Nil:
                return SymbolValue::intern(env, "nil");
            case Type::Not:
                return SymbolValue::intern(env, "not");
            case Type::Or:
                return SymbolValue::intern(env, "or");
            case Type::Plus:
                return SymbolValue::intern(env, "+");
            case Type::RBrace:
                return SymbolValue::intern(env, "}");
            case Type::RBracket:
                return SymbolValue::intern(env, "]");
            case Type::RParen:
                return SymbolValue::intern(env, ")");
            case Type::Redo:
                return SymbolValue::intern(env, "redo");
            case Type::Regexp:
                return new ArrayValue { env, { SymbolValue::intern(env, "regexp"), new StringValue { env, m_literal } } };
            case Type::Rescue:
                return SymbolValue::intern(env, "rescue");
            case Type::Retry:
                return SymbolValue::intern(env, "retry");
            case Type::Return:
                return SymbolValue::intern(env, "return");
            case Type::Self:
                return SymbolValue::intern(env, "self");
            case Type::Semicolon:
                return SymbolValue::intern(env, ";");
            case Type::String:
                return new ArrayValue { env, { SymbolValue::intern(env, "string"), new StringValue { env, m_literal } } };
            case Type::Super:
                return SymbolValue::intern(env, "super");
            case Type::Symbol:
                return new ArrayValue { env, { SymbolValue::intern(env, "symbol"), SymbolValue::intern(env, m_literal) } };
            case Type::SymbolKey:
                return new ArrayValue { env, { SymbolValue::intern(env, "symbol_key"), SymbolValue::intern(env, m_literal) } };
            case Type::Then:
                return SymbolValue::intern(env, "then");
            case Type::True:
                return SymbolValue::intern(env, "true");
            case Type::Undef:
                return SymbolValue::intern(env, "undef");
            case Type::Unless:
                return SymbolValue::intern(env, "unless");
            case Type::UnterminatedRegexp:
                env->raise("SyntaxError", "unterminated regexp meets end of file");
            case Type::UnterminatedString:
                env->raise("SyntaxError", "unterminated string meets end of file");
            case Type::Until:
                return SymbolValue::intern(env, "until");
            case Type::When:
                return SymbolValue::intern(env, "when");
            case Type::While:
                return SymbolValue::intern(env, "while");
            case Type::Yield:
                return SymbolValue::intern(env, "yield");
            }
            NAT_UNREACHABLE();
        }

        bool eof() { return m_type == Type::Eof; }
        bool valid() { return m_type != Type::Invalid; }

    private:
        Type m_type { Type::Invalid };
        const char *m_literal { nullptr };
        nat_int_t m_integer { 0 };
    };

    Vector<Token> *tokens() {
        auto tokens = new Vector<Token> {};
        for (;;) {
            auto token = next_token();
            if (token.eof())
                return tokens;
            tokens->push(token);
            if (!token.valid())
                return tokens;
        };
        NAT_UNREACHABLE();
    }

    Token next_token() {
        auto token = build_next_token();
        m_last_token = token;
        return token;
    }

    char
    current_char() {
        if (m_index >= m_size)
            return 0;
        return m_input[m_index];
    }

    bool match(size_t bytes, const char *compare) {
        if (m_index + bytes > m_size)
            return false;
        if (strncmp(compare, m_input + m_index, bytes) == 0) {
            auto index_was = m_index;
            advance(bytes);
            auto c = current_char();
            if (is_identifier_char(c)) {
                m_index = index_was;
                return false;
            }
            return true;
        }
        return false;
    }

    void advance() {
        m_index++;
    }

    void advance(size_t bytes) {
        m_index += bytes;
    }

private:
    bool is_identifier_char(char c) {
        return c && ((c >= 'a' && c <= 'z') || (c >= 'A' && c < 'Z') || (c >= '0' && c <= '9') || c == '_');
    };

    Token build_next_token() {
        auto consume_word = [this](bool *symbol_key = nullptr) {
            char c = current_char();
            auto buf = std::string("");
            do {
                buf += c;
                advance();
                c = current_char();
            } while (is_identifier_char(c));
            if (c && c == ':' && symbol_key != nullptr) {
                advance();
                *symbol_key = true;
            }
            return GC_STRDUP(buf.c_str());
        };

        auto consume_integer = [this]() {
            nat_int_t number = 0;
            do {
                number *= 10;
                number += current_char() - 48;
                advance();
            } while (isdigit(current_char()));
            // TODO: check if invalid character follows, such as a letter
            return Token { Token::Type::Integer, number };
        };

        auto consume_non_whitespace = [this]() {
            char c = current_char();
            auto buf = std::string("");
            do {
                buf += c;
                advance();
                c = current_char();
            } while (c && c != ' ' && c != '\t' && c != '\n' && c != '\r');
            return GC_STRDUP(buf.c_str());
        };

        if (m_index >= m_size)
            return Token { Token::Type::Eof };
        Token token;
        bool we_skipped_whitespace = false;
        while (current_char() == ' ' || current_char() == '\t') {
            we_skipped_whitespace = true;
            advance();
            if (m_index >= m_size)
                return Token { Token::Type::Eof };
        }
        switch (current_char()) {
        case '=': {
            advance();
            switch (current_char()) {
            case '=': {
                advance();
                switch (current_char()) {
                case '=': {
                    advance();
                    return Token { Token::Type::EqualEqualEqual };
                }
                default:
                    return Token { Token::Type::EqualEqual };
                }
            }
            case '>': {
                advance();
                return Token { Token::Type::HashRocket };
            }
            default:
                return Token { Token::Type::Equal };
            }
        }
        case '+':
            advance();
            return Token { Token::Type::Plus };
        case '-':
            advance();
            return Token { Token::Type::Minus };
        case '*':
            advance();
            return Token { Token::Type::Multiply };
        case '/': {
            auto consume_regexp = [this]() -> Token {
                auto buf = std::string("");
                char c = current_char();
                for (;;) {
                    if (!c) return Token { Token::Type::UnterminatedRegexp };
                    if (c == '/') {
                        advance();
                        return Token { Token::Type::Regexp, GC_STRDUP(buf.c_str()) };
                    }
                    buf += c;
                    advance();
                    c = current_char();
                }
                NAT_UNREACHABLE();
            };
            advance();
            switch (m_last_token.type()) {
            case Token::Type::Invalid: // no previous token
            case Token::Type::Comma:
            case Token::Type::LBrace:
            case Token::Type::LBracket:
            case Token::Type::LParen: {
                return consume_regexp();
            }
            default: {
                switch (current_char()) {
                case ' ':
                    return Token { Token::Type::Divide };
                default:
                    if (we_skipped_whitespace) {
                        return consume_regexp();
                    } else {
                        return Token { Token::Type::Divide };
                    }
                }
            }
            }
        }
        case '{':
            advance();
            return Token { Token::Type::LBrace };
        case '[':
            advance();
            return Token { Token::Type::LBracket };
        case '(':
            advance();
            return Token { Token::Type::LParen };
        case '}':
            advance();
            return Token { Token::Type::RBrace };
        case ']':
            advance();
            return Token { Token::Type::RBracket };
        case ')':
            advance();
            return Token { Token::Type::RParen };
        case '\n':
            advance();
            return Token { Token::Type::Eol };
        case ';':
            advance();
            return Token { Token::Type::Semicolon };
        case ':': {
            advance();
            const char *buf = consume_word();
            return Token { Token::Type::Symbol, buf };
        }
        case ',':
            advance();
            return Token { Token::Type::Comma };
        case '"': {
            advance();
            auto buf = std::string("");
            char c = current_char();
            for (;;) {
                if (!c) return Token { Token::Type::UnterminatedString };
                if (c == '"') {
                    advance();
                    return Token { Token::Type::String, GC_STRDUP(buf.c_str()) };
                }
                buf += c;
                advance();
                c = current_char();
            }
            NAT_UNREACHABLE();
        }
        case '\'': {
            advance();
            auto buf = std::string("");
            char c = current_char();
            for (;;) {
                if (!c) return Token { Token::Type::UnterminatedString };
                if (c == '\'') {
                    advance();
                    return Token { Token::Type::String, GC_STRDUP(buf.c_str()) };
                }
                buf += c;
                advance();
                c = current_char();
            }
            NAT_UNREACHABLE();
        }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return consume_integer();
        };
        if (!token.valid()) {
            if (match(12, "__ENCODING__"))
                return Token { Token::Type::_ENCODING_ };
            else if (match(8, "__LINE__"))
                return Token { Token::Type::_LINE_ };
            else if (match(8, "__FILE__"))
                return Token { Token::Type::_FILE_ };
            else if (match(5, "BEGIN"))
                return Token { Token::Type::BEGIN };
            else if (match(3, "END"))
                return Token { Token::Type::END };
            else if (match(5, "alias"))
                return Token { Token::Type::Alias };
            else if (match(3, "and"))
                return Token { Token::Type::And };
            else if (match(5, "begin"))
                return Token { Token::Type::Begin };
            else if (match(5, "break"))
                return Token { Token::Type::Break };
            else if (match(4, "case"))
                return Token { Token::Type::Case };
            else if (match(5, "class"))
                return Token { Token::Type::Class };
            else if (match(8, "defined?"))
                return Token { Token::Type::Defined };
            else if (match(3, "def"))
                return Token { Token::Type::Def };
            else if (match(2, "do"))
                return Token { Token::Type::Do };
            else if (match(4, "else"))
                return Token { Token::Type::Else };
            else if (match(5, "elsif"))
                return Token { Token::Type::Elsif };
            else if (match(3, "end"))
                return Token { Token::Type::End };
            else if (match(6, "ensure"))
                return Token { Token::Type::Ensure };
            else if (match(5, "false"))
                return Token { Token::Type::False };
            else if (match(3, "for"))
                return Token { Token::Type::For };
            else if (match(2, "if"))
                return Token { Token::Type::If };
            else if (match(2, "in"))
                return Token { Token::Type::In };
            else if (match(6, "module"))
                return Token { Token::Type::Module };
            else if (match(4, "next"))
                return Token { Token::Type::Next };
            else if (match(3, "nil"))
                return Token { Token::Type::Nil };
            else if (match(3, "not"))
                return Token { Token::Type::Not };
            else if (match(2, "or"))
                return Token { Token::Type::Or };
            else if (match(4, "redo"))
                return Token { Token::Type::Redo };
            else if (match(6, "rescue"))
                return Token { Token::Type::Rescue };
            else if (match(5, "retry"))
                return Token { Token::Type::Retry };
            else if (match(6, "return"))
                return Token { Token::Type::Return };
            else if (match(4, "self"))
                return Token { Token::Type::Self };
            else if (match(5, "super"))
                return Token { Token::Type::Super };
            else if (match(4, "then"))
                return Token { Token::Type::Then };
            else if (match(4, "true"))
                return Token { Token::Type::True };
            else if (match(5, "undef"))
                return Token { Token::Type::Undef };
            else if (match(6, "unless"))
                return Token { Token::Type::Unless };
            else if (match(5, "until"))
                return Token { Token::Type::Until };
            else if (match(4, "when"))
                return Token { Token::Type::When };
            else if (match(5, "while"))
                return Token { Token::Type::While };
            else if (match(5, "yield"))
                return Token { Token::Type::Yield };
            else {
                auto c = current_char();
                bool symbol_key = false;
                if ((c >= 'a' && c <= 'z') || c == '_') {
                    const char *buf = consume_word(&symbol_key);
                    if (symbol_key)
                        return Token { Token::Type::SymbolKey, buf };
                    else
                        return Token { Token::Type::Identifier, buf };
                } else if (c >= 'A' && c <= 'Z') {
                    const char *buf = consume_word(&symbol_key);
                    if (symbol_key)
                        return Token { Token::Type::SymbolKey, buf };
                    else
                        return Token { Token::Type::Constant, buf };
                } else {
                    const char *buf = consume_non_whitespace();
                    return Token { Token::Type::Invalid, buf };
                }
            }
        }
        NAT_UNREACHABLE();
    }

    const char *m_input { nullptr };
    size_t m_size { 0 };
    size_t m_index { 0 };
    Token m_last_token {};
};
}
