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
            Comment,
            Constant,
            Def,
            Defined,
            Divide,
            Do,
            Dot,
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

        Token(Type type, size_t line, size_t column)
            : m_type { type }
            , m_line { line }
            , m_column { column } { }

        Token(Type type, const char *literal, size_t line, size_t column)
            : m_type { type }
            , m_literal { literal }
            , m_line { line }
            , m_column { column } {
            assert(m_literal);
        }

        Token(Type type, nat_int_t integer, size_t line, size_t column)
            : m_type { type }
            , m_integer { integer }
            , m_line { line }
            , m_column { column } { }

        Type type() { return m_type; }
        const char *literal() { return m_literal; }
        nat_int_t integer() { return m_integer; }

        Value *to_ruby(Env *env, bool with_line_and_column_numbers = false) {
            Value *type;
            switch (m_type) {
            case Type::_ENCODING_:
                type = SymbolValue::intern(env, "__ENCODING__");
                break;
            case Type::_FILE_:
                type = SymbolValue::intern(env, "__FILE__");
                break;
            case Type::_LINE_:
                type = SymbolValue::intern(env, "__LINE__");
                break;
            case Type::Alias:
                type = SymbolValue::intern(env, "alias");
                break;
            case Type::And:
                type = SymbolValue::intern(env, "and");
                break;
            case Type::Begin:
                type = SymbolValue::intern(env, "begin");
                break;
            case Type::BEGIN:
                type = SymbolValue::intern(env, "BEGIN");
                break;
            case Type::Break:
                type = SymbolValue::intern(env, "break");
                break;
            case Type::Case:
                type = SymbolValue::intern(env, "case");
                break;
            case Type::Class:
                type = SymbolValue::intern(env, "class");
                break;
            case Type::Comma:
                type = SymbolValue::intern(env, ",");
                break;
            case Type::Comment:
                type = env->nil_obj();
                break;
            case Type::Constant:
                type = SymbolValue::intern(env, "constant");
                break;
            case Type::Def:
                type = SymbolValue::intern(env, "def");
                break;
            case Type::Defined:
                type = SymbolValue::intern(env, "defined?");
                break;
            case Type::Divide:
                type = SymbolValue::intern(env, "/");
                break;
            case Type::Do:
                type = SymbolValue::intern(env, "do");
                break;
            case Type::Dot:
                type = SymbolValue::intern(env, ".");
                break;
            case Type::End:
                type = SymbolValue::intern(env, "end");
                break;
            case Type::Else:
                type = SymbolValue::intern(env, "else");
                break;
            case Type::Elsif:
                type = SymbolValue::intern(env, "elsif");
                break;
            case Type::END:
                type = SymbolValue::intern(env, "END");
                break;
            case Type::Ensure:
                type = SymbolValue::intern(env, "ensure");
                break;
            case Type::Eol:
                type = SymbolValue::intern(env, "\n");
                break;
            case Type::Eof:
                type = env->nil_obj();
                break;
            case Type::Equal:
                type = SymbolValue::intern(env, "=");
                break;
            case Type::EqualEqual:
                type = SymbolValue::intern(env, "==");
                break;
            case Type::EqualEqualEqual:
                type = SymbolValue::intern(env, "===");
                break;
            case Type::Fail:
                type = SymbolValue::intern(env, "fail");
                break;
            case Type::False:
                type = SymbolValue::intern(env, "false");
                break;
            case Type::For:
                type = SymbolValue::intern(env, "for");
                break;
            case Type::HashRocket:
                type = SymbolValue::intern(env, "=>");
                break;
            case Type::Identifier:
                type = SymbolValue::intern(env, "identifier");
                break;
            case Type::If:
                type = SymbolValue::intern(env, "if");
                break;
            case Type::In:
                type = SymbolValue::intern(env, "in");
                break;
            case Type::Integer:
                type = SymbolValue::intern(env, "integer");
                break;
            case Type::Invalid:
                env->raise("SyntaxError", "syntax error, unexpected '%s'", m_literal);
                break;
            case Type::LBrace:
                type = SymbolValue::intern(env, "{");
                break;
            case Type::LBracket:
                type = SymbolValue::intern(env, "[");
                break;
            case Type::LParen:
                type = SymbolValue::intern(env, "(");
                break;
            case Type::Loop:
                type = SymbolValue::intern(env, "loop");
                break;
            case Type::Minus:
                type = SymbolValue::intern(env, "-");
                break;
            case Type::Module:
                type = SymbolValue::intern(env, "module");
                break;
            case Type::Multiply:
                type = SymbolValue::intern(env, "*");
                break;
            case Type::Next:
                type = SymbolValue::intern(env, "next");
                break;
            case Type::Nil:
                type = SymbolValue::intern(env, "nil");
                break;
            case Type::Not:
                type = SymbolValue::intern(env, "not");
                break;
            case Type::Or:
                type = SymbolValue::intern(env, "or");
                break;
            case Type::Plus:
                type = SymbolValue::intern(env, "+");
                break;
            case Type::RBrace:
                type = SymbolValue::intern(env, "}");
                break;
            case Type::RBracket:
                type = SymbolValue::intern(env, "]");
                break;
            case Type::RParen:
                type = SymbolValue::intern(env, ")");
                break;
            case Type::Redo:
                type = SymbolValue::intern(env, "redo");
                break;
            case Type::Regexp:
                type = SymbolValue::intern(env, "regexp");
                break;
            case Type::Rescue:
                type = SymbolValue::intern(env, "rescue");
                break;
            case Type::Retry:
                type = SymbolValue::intern(env, "retry");
                break;
            case Type::Return:
                type = SymbolValue::intern(env, "return");
                break;
            case Type::Self:
                type = SymbolValue::intern(env, "self");
                break;
            case Type::Semicolon:
                type = SymbolValue::intern(env, ";");
                break;
            case Type::String:
                type = SymbolValue::intern(env, "string");
                break;
            case Type::Super:
                type = SymbolValue::intern(env, "super");
                break;
            case Type::Symbol:
                type = SymbolValue::intern(env, "symbol");
                break;
            case Type::SymbolKey:
                type = SymbolValue::intern(env, "symbol_key");
                break;
            case Type::Then:
                type = SymbolValue::intern(env, "then");
                break;
            case Type::True:
                type = SymbolValue::intern(env, "true");
                break;
            case Type::Undef:
                type = SymbolValue::intern(env, "undef");
                break;
            case Type::Unless:
                type = SymbolValue::intern(env, "unless");
                break;
            case Type::UnterminatedRegexp:
                env->raise("SyntaxError", "unterminated regexp meets end of file");
            case Type::UnterminatedString:
                env->raise("SyntaxError", "unterminated string meets end of file");
            case Type::Until:
                type = SymbolValue::intern(env, "until");
                break;
            case Type::When:
                type = SymbolValue::intern(env, "when");
                break;
            case Type::While:
                type = SymbolValue::intern(env, "while");
                break;
            case Type::Yield:
                type = SymbolValue::intern(env, "yield");
                break;
            }
            auto hash = new HashValue { env };
            hash->put(env, SymbolValue::intern(env, "type"), type);
            switch (m_type) {
            case Type::Regexp:
            case Type::String:
                hash->put(env, SymbolValue::intern(env, "literal"), new StringValue { env, m_literal });
                break;
            case Type::Constant:
            case Type::Identifier:
            case Type::Symbol:
            case Type::SymbolKey:
                hash->put(env, SymbolValue::intern(env, "literal"), SymbolValue::intern(env, m_literal));
                break;
            case Type::Integer:
                hash->put(env, SymbolValue::intern(env, "literal"), new IntegerValue { env, m_integer });
                break;
            default:
                void();
            }
            if (with_line_and_column_numbers) {
                hash->put(env, SymbolValue::intern(env, "line"), new IntegerValue { env, static_cast<nat_int_t>(m_line) });
                hash->put(env, SymbolValue::intern(env, "column"), new IntegerValue { env, static_cast<nat_int_t>(m_column) });
            }
            return hash;
        }

        bool is_comment() { return m_type == Type::Comment; }
        bool is_eof() { return m_type == Type::Eof; }
        bool is_valid() { return m_type != Type::Invalid; }

        size_t line() { return m_line; }
        size_t column() { return m_column; }

    private:
        Type m_type { Type::Invalid };
        const char *m_literal { nullptr };
        nat_int_t m_integer { 0 };
        size_t m_line { 0 };
        size_t m_column { 0 };
    };

    Vector<Token> *tokens() {
        auto tokens = new Vector<Token> {};
        for (;;) {
            auto token = next_token();
            if (token.is_eof())
                return tokens;
            if (token.is_comment())
                continue;
            tokens->push(token);
            if (!token.is_valid())
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
            if (m_index + bytes < m_size && is_identifier_char(m_input[m_index + bytes]))
                return false;
            advance(bytes);
            return true;
        }
        return false;
    }

    void advance() {
        auto c = current_char();
        m_index++;
        if (c == '\n') {
            m_line++;
            m_column = 0;
        } else {
            m_column++;
        }
    }

    void advance(size_t bytes) {
        for (size_t i = 0; i < bytes; i++) {
            advance();
        }
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
            return number;
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
            return Token { Token::Type::Eof, m_line, m_column };
        Token token;
        bool we_skipped_whitespace = false;
        while (current_char() == ' ' || current_char() == '\t') {
            we_skipped_whitespace = true;
            advance();
            if (m_index >= m_size)
                return Token { Token::Type::Eof, m_line, m_column };
        }
        auto line = m_line;
        auto column = m_column;
        switch (current_char()) {
        case '=': {
            advance();
            switch (current_char()) {
            case '=': {
                advance();
                switch (current_char()) {
                case '=': {
                    advance();
                    return Token { Token::Type::EqualEqualEqual, line, column };
                }
                default:
                    return Token { Token::Type::EqualEqual, line, column };
                }
            }
            case '>': {
                advance();
                return Token { Token::Type::HashRocket, line, column };
            }
            default:
                return Token { Token::Type::Equal, line, column };
            }
        }
        case '+':
            advance();
            return Token { Token::Type::Plus, line, column };
        case '-':
            advance();
            return Token { Token::Type::Minus, line, column };
        case '*':
            advance();
            return Token { Token::Type::Multiply, line, column };
        case '/': {
            auto consume_regexp = [this, line, column]() -> Token {
                auto buf = std::string("");
                char c = current_char();
                for (;;) {
                    if (!c) return Token { Token::Type::UnterminatedRegexp, line, column };
                    if (c == '/') {
                        advance();
                        return Token { Token::Type::Regexp, GC_STRDUP(buf.c_str()), line, column };
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
                    return Token { Token::Type::Divide, line, column };
                default:
                    if (we_skipped_whitespace) {
                        return consume_regexp();
                    } else {
                        return Token { Token::Type::Divide, line, column };
                    }
                }
            }
            }
        }
        case '.':
            advance();
            return Token { Token::Type::Dot, line, column };
        case '{':
            advance();
            return Token { Token::Type::LBrace, line, column };
        case '[':
            advance();
            return Token { Token::Type::LBracket, line, column };
        case '(':
            advance();
            return Token { Token::Type::LParen, line, column };
        case '}':
            advance();
            return Token { Token::Type::RBrace, line, column };
        case ']':
            advance();
            return Token { Token::Type::RBracket, line, column };
        case ')':
            advance();
            return Token { Token::Type::RParen, line, column };
        case '\n':
            advance();
            return Token { Token::Type::Eol, line, column };
        case ';':
            advance();
            return Token { Token::Type::Semicolon, line, column };
        case ':': {
            advance();
            const char *buf = consume_word();
            return Token { Token::Type::Symbol, buf, line, column };
        }
        case ',':
            advance();
            return Token { Token::Type::Comma, line, column };
        case '"': {
            advance();
            auto buf = std::string("");
            char c = current_char();
            for (;;) {
                if (!c) return Token { Token::Type::UnterminatedString, line, column };
                if (c == '"') {
                    advance();
                    return Token { Token::Type::String, GC_STRDUP(buf.c_str()), line, column };
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
                if (!c) return Token { Token::Type::UnterminatedString, line, column };
                if (c == '\'') {
                    advance();
                    return Token { Token::Type::String, GC_STRDUP(buf.c_str()), line, column };
                }
                buf += c;
                advance();
                c = current_char();
            }
            NAT_UNREACHABLE();
        }
        case '#':
            char c;
            do {
                advance();
                c = current_char();
            } while (c != '\n' && c != '\r');
            return Token { Token::Type::Comment, line, column };
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
            return Token { Token::Type::Integer, consume_integer(), line, column };
        };
        if (!token.is_valid()) {
            if (match(12, "__ENCODING__"))
                return Token { Token::Type::_ENCODING_, line, column };
            else if (match(8, "__LINE__"))
                return Token { Token::Type::_LINE_, line, column };
            else if (match(8, "__FILE__"))
                return Token { Token::Type::_FILE_, line, column };
            else if (match(5, "BEGIN"))
                return Token { Token::Type::BEGIN, line, column };
            else if (match(3, "END"))
                return Token { Token::Type::END, line, column };
            else if (match(5, "alias"))
                return Token { Token::Type::Alias, line, column };
            else if (match(3, "and"))
                return Token { Token::Type::And, line, column };
            else if (match(5, "begin"))
                return Token { Token::Type::Begin, line, column };
            else if (match(5, "break"))
                return Token { Token::Type::Break, line, column };
            else if (match(4, "case"))
                return Token { Token::Type::Case, line, column };
            else if (match(5, "class"))
                return Token { Token::Type::Class, line, column };
            else if (match(8, "defined?"))
                return Token { Token::Type::Defined, line, column };
            else if (match(3, "def"))
                return Token { Token::Type::Def, line, column };
            else if (match(2, "do"))
                return Token { Token::Type::Do, line, column };
            else if (match(4, "else"))
                return Token { Token::Type::Else, line, column };
            else if (match(5, "elsif"))
                return Token { Token::Type::Elsif, line, column };
            else if (match(3, "end"))
                return Token { Token::Type::End, line, column };
            else if (match(6, "ensure"))
                return Token { Token::Type::Ensure, line, column };
            else if (match(5, "false"))
                return Token { Token::Type::False, line, column };
            else if (match(3, "for"))
                return Token { Token::Type::For, line, column };
            else if (match(2, "if"))
                return Token { Token::Type::If, line, column };
            else if (match(2, "in"))
                return Token { Token::Type::In, line, column };
            else if (match(6, "module"))
                return Token { Token::Type::Module, line, column };
            else if (match(4, "next"))
                return Token { Token::Type::Next, line, column };
            else if (match(3, "nil"))
                return Token { Token::Type::Nil, line, column };
            else if (match(3, "not"))
                return Token { Token::Type::Not, line, column };
            else if (match(2, "or"))
                return Token { Token::Type::Or, line, column };
            else if (match(4, "redo"))
                return Token { Token::Type::Redo, line, column };
            else if (match(6, "rescue"))
                return Token { Token::Type::Rescue, line, column };
            else if (match(5, "retry"))
                return Token { Token::Type::Retry, line, column };
            else if (match(6, "return"))
                return Token { Token::Type::Return, line, column };
            else if (match(4, "self"))
                return Token { Token::Type::Self, line, column };
            else if (match(5, "super"))
                return Token { Token::Type::Super, line, column };
            else if (match(4, "then"))
                return Token { Token::Type::Then, line, column };
            else if (match(4, "true"))
                return Token { Token::Type::True, line, column };
            else if (match(5, "undef"))
                return Token { Token::Type::Undef, line, column };
            else if (match(6, "unless"))
                return Token { Token::Type::Unless, line, column };
            else if (match(5, "until"))
                return Token { Token::Type::Until, line, column };
            else if (match(4, "when"))
                return Token { Token::Type::When, line, column };
            else if (match(5, "while"))
                return Token { Token::Type::While, line, column };
            else if (match(5, "yield"))
                return Token { Token::Type::Yield, line, column };
            else {
                auto c = current_char();
                bool symbol_key = false;
                if ((c >= 'a' && c <= 'z') || c == '_') {
                    const char *buf = consume_word(&symbol_key);
                    if (symbol_key)
                        return Token { Token::Type::SymbolKey, buf, line, column };
                    else
                        return Token { Token::Type::Identifier, buf, line, column };
                } else if (c >= 'A' && c <= 'Z') {
                    const char *buf = consume_word(&symbol_key);
                    if (symbol_key)
                        return Token { Token::Type::SymbolKey, buf, line, column };
                    else
                        return Token { Token::Type::Constant, buf, line, column };
                } else {
                    const char *buf = consume_non_whitespace();
                    return Token { Token::Type::Invalid, buf, line, column };
                }
            }
        }
        NAT_UNREACHABLE();
    }

    const char *m_input { nullptr };
    size_t m_size { 0 };
    size_t m_index { 0 };
    size_t m_line { 0 };
    size_t m_column { 0 };
    Token m_last_token {};
};
}
