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
            And,
            Arrow,
            BinaryAnd,
            BinaryOr,
            BinaryXor,
            BinaryOnesComplement,
            ClassVariable,
            Comma,
            Comment,
            Comparison,
            Constant,
            ConstantResolution,
            Divide,
            DivideEqual,
            Dot,
            DotDot,
            Eof,
            Eol,
            Equal,
            EqualEqual,
            EqualEqualEqual,
            Exponent,
            ExponentEqual,
            GreaterThan,
            GreaterThanOrEqual,
            GlobalVariable,
            HashRocket,
            Identifier,
            InstanceVariable,
            Integer,
            Invalid,
            Keyword,
            LBrace,
            LBracket,
            LParen,
            LeftShift,
            LessThan,
            LessThanOrEqual,
            Match,
            Minus,
            MinusEqual,
            Modulus,
            ModulusEqual,
            Multiply,
            MultiplyEqual,
            Not,
            NotEqual,
            NotMatch,
            Or,
            PercentLowerI,
            PercentLowerW,
            PercentUpperI,
            PercentUpperW,
            Plus,
            PlusEqual,
            RBrace,
            RBracket,
            Regexp,
            RightShift,
            RParen,
            Semicolon,
            String,
            Symbol,
            SymbolKey,
            TernaryColon,
            TernaryQuestion,
            UnterminatedRegexp,
            UnterminatedString,
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
        void set_type(Token::Type type) { m_type = type; }

        const char *literal() { return m_literal; }
        void set_literal(const char *literal) { m_literal = literal; }
        void set_literal(std::string literal) { m_literal = GC_STRDUP(literal.c_str()); }

        nat_int_t integer() { return m_integer; }

        Value *to_ruby(Env *env, bool with_line_and_column_numbers = false) {
            Value *type;
            switch (m_type) {
            case Type::And:
                type = SymbolValue::intern(env, "&&");
                break;
            case Type::Arrow:
                type = SymbolValue::intern(env, "->");
                break;
            case Type::BinaryAnd:
                type = SymbolValue::intern(env, "&");
                break;
            case Type::BinaryOr:
                type = SymbolValue::intern(env, "|");
                break;
            case Type::BinaryXor:
                type = SymbolValue::intern(env, "^");
                break;
            case Type::BinaryOnesComplement:
                type = SymbolValue::intern(env, "~");
                break;
            case Type::ClassVariable:
                type = SymbolValue::intern(env, "cvar");
                break;
            case Type::Comma:
                type = SymbolValue::intern(env, ",");
                break;
            case Type::Comment:
                type = env->nil_obj();
                break;
            case Type::Comparison:
                type = SymbolValue::intern(env, "<=>");
                break;
            case Type::Constant:
                type = SymbolValue::intern(env, "constant");
                break;
            case Type::ConstantResolution:
                type = SymbolValue::intern(env, "::");
                break;
            case Type::Divide:
                type = SymbolValue::intern(env, "/");
                break;
            case Type::DivideEqual:
                type = SymbolValue::intern(env, "/=");
                break;
            case Type::Dot:
                type = SymbolValue::intern(env, ".");
                break;
            case Type::DotDot:
                type = SymbolValue::intern(env, "..");
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
            case Type::Exponent:
                type = SymbolValue::intern(env, "**");
                break;
            case Type::ExponentEqual:
                type = SymbolValue::intern(env, "**=");
                break;
            case Type::GlobalVariable:
                type = SymbolValue::intern(env, "gvar");
                break;
            case Type::GreaterThan:
                type = SymbolValue::intern(env, ">");
                break;
            case Type::GreaterThanOrEqual:
                type = SymbolValue::intern(env, ">=");
                break;
            case Type::HashRocket:
                type = SymbolValue::intern(env, "=>");
                break;
            case Type::Identifier:
                type = SymbolValue::intern(env, "identifier");
                break;
            case Type::InstanceVariable:
                type = SymbolValue::intern(env, "ivar");
                break;
            case Type::Integer:
                type = SymbolValue::intern(env, "integer");
                break;
            case Type::Invalid:
                env->raise("SyntaxError", "syntax error, unexpected '%s'", m_literal);
                break;
            case Type::Keyword:
                type = SymbolValue::intern(env, "keyword");
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
            case Type::LeftShift:
                type = SymbolValue::intern(env, "<<");
                break;
            case Type::LessThan:
                type = SymbolValue::intern(env, "<");
                break;
            case Type::LessThanOrEqual:
                type = SymbolValue::intern(env, "<=");
                break;
            case Type::Match:
                type = SymbolValue::intern(env, "=~");
                break;
            case Type::Minus:
                type = SymbolValue::intern(env, "-");
                break;
            case Type::MinusEqual:
                type = SymbolValue::intern(env, "-=");
                break;
            case Type::Modulus:
                type = SymbolValue::intern(env, "%");
                break;
            case Type::ModulusEqual:
                type = SymbolValue::intern(env, "%=");
                break;
            case Type::Multiply:
                type = SymbolValue::intern(env, "*");
                break;
            case Type::MultiplyEqual:
                type = SymbolValue::intern(env, "*=");
                break;
            case Type::Not:
                type = SymbolValue::intern(env, "!");
                break;
            case Type::NotEqual:
                type = SymbolValue::intern(env, "!=");
                break;
            case Type::NotMatch:
                type = SymbolValue::intern(env, "!~");
                break;
            case Type::Or:
                type = SymbolValue::intern(env, "||");
                break;
            case Type::PercentLowerI:
                type = SymbolValue::intern(env, "%i");
                break;
            case Type::PercentUpperI:
                type = SymbolValue::intern(env, "%I");
                break;
            case Type::PercentLowerW:
                type = SymbolValue::intern(env, "%w");
                break;
            case Type::PercentUpperW:
                type = SymbolValue::intern(env, "%W");
                break;
            case Type::Plus:
                type = SymbolValue::intern(env, "+");
                break;
            case Type::PlusEqual:
                type = SymbolValue::intern(env, "+=");
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
            case Type::Regexp:
                type = SymbolValue::intern(env, "regexp");
                break;
            case Type::RightShift:
                type = SymbolValue::intern(env, ">>");
                break;
            case Type::Semicolon:
                type = SymbolValue::intern(env, ";");
                break;
            case Type::String:
                type = SymbolValue::intern(env, "string");
                break;
            case Type::Symbol:
                type = SymbolValue::intern(env, "symbol");
                break;
            case Type::SymbolKey:
                type = SymbolValue::intern(env, "symbol_key");
                break;
            case Type::TernaryColon:
                type = SymbolValue::intern(env, ":");
                break;
            case Type::TernaryQuestion:
                type = SymbolValue::intern(env, "?");
                break;
            case Type::UnterminatedRegexp:
                env->raise("SyntaxError", "unterminated regexp meets end of file");
            case Type::UnterminatedString:
                env->raise("SyntaxError", "unterminated string meets end of file at line %i and column %i: %s", m_line, m_column, m_literal);
            }
            auto hash = new HashValue { env };
            hash->put(env, SymbolValue::intern(env, "type"), type);
            switch (m_type) {
            case Type::PercentLowerI:
            case Type::PercentUpperI:
            case Type::PercentLowerW:
            case Type::PercentUpperW:
            case Type::Regexp:
            case Type::String:
                hash->put(env, SymbolValue::intern(env, "literal"), new StringValue { env, m_literal });
                break;
            case Type::ClassVariable:
            case Type::Constant:
            case Type::GlobalVariable:
            case Type::Keyword:
            case Type::Identifier:
            case Type::InstanceVariable:
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

        // FIXME: make "def" its own type and eliminate this strcmp
        bool can_precede_method_name() {
            return m_type == Token::Type::Dot || m_type == Token::Type::ConstantResolution || (m_type == Token::Type::Keyword && strcmp(m_literal, "def") == 0);
        }

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
        m_whitespace_precedes = skip_whitespace();
        m_token_line = m_cursor_line;
        m_token_column = m_cursor_column;
        auto token = build_next_token();
        m_last_token = token;
        return token;
    }

    char current_char() {
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
            m_cursor_line++;
            m_cursor_column = 0;
        } else {
            m_cursor_column++;
        }
    }

    void advance(size_t bytes) {
        for (size_t i = 0; i < bytes; i++) {
            advance();
        }
    }

    char peek() {
        if (m_index + 1 >= m_size)
            return 0;
        return m_input[m_index + 1];
    }

private:
    bool is_identifier_char(char c) {
        return c && ((c >= 'a' && c <= 'z') || (c >= 'A' && c < 'Z') || (c >= '0' && c <= '9') || c == '_');
    };

    bool skip_whitespace() {
        bool whitespace_found = false;
        while (current_char() == ' ' || current_char() == '\t') {
            whitespace_found = true;
            advance();
        }
        return whitespace_found;
    }

    Token build_next_token() {
        if (m_index >= m_size)
            return Token { Token::Type::Eof, m_token_line, m_token_column };
        Token token;
        switch (current_char()) {
        case '=': {
            advance();
            switch (current_char()) {
            case '=': {
                advance();
                switch (current_char()) {
                case '=': {
                    advance();
                    return Token { Token::Type::EqualEqualEqual, m_token_line, m_token_column };
                }
                default:
                    return Token { Token::Type::EqualEqual, m_token_line, m_token_column };
                }
            }
            case '>':
                advance();
                return Token { Token::Type::HashRocket, m_token_line, m_token_column };
            case '~':
                advance();
                return Token { Token::Type::Match, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Equal, m_token_line, m_token_column };
            }
        }
        case '+':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::PlusEqual, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Plus, m_token_line, m_token_column };
            }
        case '-':
            advance();
            switch (current_char()) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                const bool is_negative = true;
                return consume_integer(is_negative);
            }
            case '>':
                advance();
                return Token { Token::Type::Arrow, m_token_line, m_token_column };
            default:
                switch (current_char()) {
                case '=':
                    advance();
                    return Token { Token::Type::MinusEqual, m_token_line, m_token_column };
                default:
                    return Token { Token::Type::Minus, m_token_line, m_token_column };
                }
            }
        case '*':
            advance();
            switch (current_char()) {
            case '*':
                advance();
                switch (current_char()) {
                case '=':
                    advance();
                    return Token { Token::Type::ExponentEqual, m_token_line, m_token_column };
                default:
                    return Token { Token::Type::Exponent, m_token_line, m_token_column };
                }
            case '=':
                advance();
                return Token { Token::Type::MultiplyEqual, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Multiply, m_token_line, m_token_column };
            }
        case '/': {
            advance();
            switch (m_last_token.type()) {
            case Token::Type::Invalid: // no previous token
            case Token::Type::Comma:
            case Token::Type::LBrace:
            case Token::Type::LBracket:
            case Token::Type::LParen:
                return consume_regexp();
            default: {
                switch (current_char()) {
                case ' ':
                    return Token { Token::Type::Divide, m_token_line, m_token_column };
                case '=':
                    advance();
                    return Token { Token::Type::DivideEqual, m_token_line, m_token_column };
                default:
                    if (m_whitespace_precedes) {
                        return consume_regexp();
                    } else {
                        return Token { Token::Type::Divide, m_token_line, m_token_column };
                    }
                }
            }
            }
        }
        case '%':
            advance();
            if (current_char() == 'q')
                advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::ModulusEqual, m_token_line, m_token_column };
            case '/':
                advance();
                return consume_single_quoted_string('/');
            case '[':
                advance();
                return consume_single_quoted_string(']');
            case '{':
                advance();
                return consume_single_quoted_string('}');
            case '(':
                advance();
                return consume_single_quoted_string(')');
            case 'Q':
                switch (peek()) {
                case '/':
                    advance(2);
                    return consume_double_quoted_string('/');
                case '[':
                    advance(2);
                    return consume_double_quoted_string(']');
                case '{':
                    advance(2);
                    return consume_double_quoted_string('}');
                case '(':
                    advance(2);
                    return consume_double_quoted_string(')');
                default:
                    return Token { Token::Type::Modulus, m_token_line, m_token_column };
                }
            case 'w':
                switch (peek()) {
                case '/':
                    advance(2);
                    return consume_quoted_array_without_interpolation('/', Token::Type::PercentLowerW);
                case '[':
                    advance(2);
                    return consume_quoted_array_without_interpolation(']', Token::Type::PercentLowerW);
                case '{':
                    advance(2);
                    return consume_quoted_array_without_interpolation('}', Token::Type::PercentLowerW);
                case '(':
                    advance(2);
                    return consume_quoted_array_without_interpolation(')', Token::Type::PercentLowerW);
                default:
                    return Token { Token::Type::Modulus, m_token_line, m_token_column };
                }
            case 'W':
                switch (peek()) {
                case '/':
                    advance(2);
                    return consume_quoted_array_with_interpolation('/', Token::Type::PercentUpperW);
                case '[':
                    advance(2);
                    return consume_quoted_array_with_interpolation(']', Token::Type::PercentUpperW);
                case '{':
                    advance(2);
                    return consume_quoted_array_with_interpolation('}', Token::Type::PercentUpperW);
                case '(':
                    advance(2);
                    return consume_quoted_array_with_interpolation(')', Token::Type::PercentUpperW);
                default:
                    return Token { Token::Type::Modulus, m_token_line, m_token_column };
                }
            case 'i':
                switch (peek()) {
                case '/':
                    advance(2);
                    return consume_quoted_array_without_interpolation('/', Token::Type::PercentLowerI);
                case '[':
                    advance(2);
                    return consume_quoted_array_without_interpolation(']', Token::Type::PercentLowerI);
                case '{':
                    advance(2);
                    return consume_quoted_array_without_interpolation('}', Token::Type::PercentLowerI);
                case '(':
                    advance(2);
                    return consume_quoted_array_without_interpolation(')', Token::Type::PercentLowerI);
                default:
                    return Token { Token::Type::Modulus, m_token_line, m_token_column };
                }
            case 'I':
                switch (peek()) {
                case '/':
                    advance(2);
                    return consume_quoted_array_with_interpolation('/', Token::Type::PercentUpperI);
                case '[':
                    advance(2);
                    return consume_quoted_array_with_interpolation(']', Token::Type::PercentUpperI);
                case '{':
                    advance(2);
                    return consume_quoted_array_with_interpolation('}', Token::Type::PercentUpperI);
                case '(':
                    advance(2);
                    return consume_quoted_array_with_interpolation(')', Token::Type::PercentUpperI);
                default:
                    return Token { Token::Type::Modulus, m_token_line, m_token_column };
                }
            default:
                return Token { Token::Type::Modulus, m_token_line, m_token_column };
            }
        case '!':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::NotEqual, m_token_line, m_token_column };
            case '~':
                advance();
                return Token { Token::Type::NotMatch, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Not, m_token_line, m_token_column };
            }
        case '<':
            advance();
            switch (current_char()) {
            case '<':
                advance();
                return Token { Token::Type::LeftShift, m_token_line, m_token_column };
            case '=':
                advance();
                switch (current_char()) {
                case '>':
                    advance();
                    return Token { Token::Type::Comparison, m_token_line, m_token_column };
                default:
                    return Token { Token::Type::LessThanOrEqual, m_token_line, m_token_column };
                }
            default:
                return Token { Token::Type::LessThan, m_token_line, m_token_column };
            }
        case '>':
            advance();
            switch (current_char()) {
            case '>':
                advance();
                return Token { Token::Type::RightShift, m_token_line, m_token_column };
            case '=':
                advance();
                return Token { Token::Type::GreaterThanOrEqual, m_token_line, m_token_column };
            default:
                return Token { Token::Type::GreaterThan, m_token_line, m_token_column };
            }
        case '&':
            advance();
            switch (current_char()) {
            case '&':
                advance();
                return Token { Token::Type::And, m_token_line, m_token_column };
            default:
                return Token { Token::Type::BinaryAnd, m_token_line, m_token_column };
            }
        case '|':
            advance();
            switch (current_char()) {
            case '|':
                advance();
                return Token { Token::Type::Or, m_token_line, m_token_column };
            default:
                return Token { Token::Type::BinaryOr, m_token_line, m_token_column };
            }
        case '^':
            advance();
            return Token { Token::Type::BinaryXor, m_token_line, m_token_column };
        case '~':
            advance();
            return Token { Token::Type::BinaryOnesComplement, m_token_line, m_token_column };
        case '?':
            advance();
            return Token { Token::Type::TernaryQuestion, m_token_line, m_token_column };
        case ':': {
            advance();
            auto c = current_char();
            if (is_identifier_char(c)) {
                return consume_word(Token::Type::Symbol);
            } else if (c == ':') {
                advance();
                return Token { Token::Type::ConstantResolution, m_token_line, m_token_column };
            } else {
                return Token { Token::Type::TernaryColon, m_token_line, m_token_column };
            }
        }
        case '@':
            switch (peek()) {
            case '@': {
                // kinda janky, but we gotta trick consume_word and then prepend the '@' back on the front
                advance();
                auto token = consume_word(Token::Type::ClassVariable);
                token.set_literal(std::string(1, '@') + std::string(token.literal()));
                return token;
            }
            default:
                return consume_word(Token::Type::InstanceVariable);
            }
        case '$': {
            return consume_word(Token::Type::GlobalVariable);
        }
        case '.':
            advance();
            switch (current_char()) {
            case '.':
                advance();
                return Token { Token::Type::DotDot, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Dot, m_token_line, m_token_column };
            }
        case '{':
            advance();
            return Token { Token::Type::LBrace, m_token_line, m_token_column };
        case '[':
            advance();
            return Token { Token::Type::LBracket, m_token_line, m_token_column };
        case '(':
            advance();
            return Token { Token::Type::LParen, m_token_line, m_token_column };
        case '}':
            advance();
            return Token { Token::Type::RBrace, m_token_line, m_token_column };
        case ']':
            advance();
            return Token { Token::Type::RBracket, m_token_line, m_token_column };
        case ')':
            advance();
            return Token { Token::Type::RParen, m_token_line, m_token_column };
        case '\n':
            advance();
            return Token { Token::Type::Eol, m_token_line, m_token_column };
        case ';':
            advance();
            return Token { Token::Type::Semicolon, m_token_line, m_token_column };
        case ',':
            advance();
            return Token { Token::Type::Comma, m_token_line, m_token_column };
        case '"': {
            advance();
            return consume_double_quoted_string('"');
        }
        case '\'': {
            advance();
            return consume_single_quoted_string('\'');
        }
        case '#':
            char c;
            do {
                advance();
                c = current_char();
            } while (c && c != '\n' && c != '\r');
            return Token { Token::Type::Comment, m_token_line, m_token_column };
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
            return consume_integer(false);
        };
        if (!token.is_valid()) {
            if (match(12, "__ENCODING__"))
                return Token { Token::Type::Keyword, GC_STRDUP("__ENCODING__"), m_token_line, m_token_column };
            else if (match(8, "__LINE__"))
                return Token { Token::Type::Keyword, GC_STRDUP("__LINE__"), m_token_line, m_token_column };
            else if (match(8, "__FILE__"))
                return Token { Token::Type::Keyword, GC_STRDUP("__FILE__"), m_token_line, m_token_column };
            else if (match(5, "BEGIN"))
                return Token { Token::Type::Keyword, GC_STRDUP("BEGIN"), m_token_line, m_token_column };
            else if (match(3, "END"))
                return Token { Token::Type::Keyword, GC_STRDUP("END"), m_token_line, m_token_column };
            else if (match(5, "alias"))
                return Token { Token::Type::Keyword, GC_STRDUP("alias"), m_token_line, m_token_column };
            else if (match(3, "and"))
                return Token { Token::Type::Keyword, GC_STRDUP("and"), m_token_line, m_token_column };
            else if (match(5, "begin"))
                return Token { Token::Type::Keyword, GC_STRDUP("begin"), m_token_line, m_token_column };
            else if (match(5, "break"))
                return Token { Token::Type::Keyword, GC_STRDUP("break"), m_token_line, m_token_column };
            else if (match(4, "case"))
                return Token { Token::Type::Keyword, GC_STRDUP("case"), m_token_line, m_token_column };
            else if (match(5, "class"))
                return Token { Token::Type::Keyword, GC_STRDUP("class"), m_token_line, m_token_column };
            else if (match(8, "defined?"))
                return Token { Token::Type::Keyword, GC_STRDUP("defined?"), m_token_line, m_token_column };
            else if (match(3, "def"))
                return Token { Token::Type::Keyword, GC_STRDUP("def"), m_token_line, m_token_column };
            else if (match(2, "do"))
                return Token { Token::Type::Keyword, GC_STRDUP("do"), m_token_line, m_token_column };
            else if (match(4, "else"))
                return Token { Token::Type::Keyword, GC_STRDUP("else"), m_token_line, m_token_column };
            else if (match(5, "elsif"))
                return Token { Token::Type::Keyword, GC_STRDUP("elsif"), m_token_line, m_token_column };
            else if (match(3, "end"))
                return Token { Token::Type::Keyword, GC_STRDUP("end"), m_token_line, m_token_column };
            else if (match(6, "ensure"))
                return Token { Token::Type::Keyword, GC_STRDUP("ensure"), m_token_line, m_token_column };
            else if (match(5, "false"))
                return Token { Token::Type::Keyword, GC_STRDUP("false"), m_token_line, m_token_column };
            else if (match(3, "for"))
                return Token { Token::Type::Keyword, GC_STRDUP("for"), m_token_line, m_token_column };
            else if (match(2, "if"))
                return Token { Token::Type::Keyword, GC_STRDUP("if"), m_token_line, m_token_column };
            else if (match(2, "in"))
                return Token { Token::Type::Keyword, GC_STRDUP("in"), m_token_line, m_token_column };
            else if (match(6, "module"))
                return Token { Token::Type::Keyword, GC_STRDUP("module"), m_token_line, m_token_column };
            else if (match(4, "next"))
                return Token { Token::Type::Keyword, GC_STRDUP("next"), m_token_line, m_token_column };
            else if (match(3, "nil"))
                return Token { Token::Type::Keyword, GC_STRDUP("nil"), m_token_line, m_token_column };
            else if (match(3, "not"))
                return Token { Token::Type::Keyword, GC_STRDUP("not"), m_token_line, m_token_column };
            else if (match(2, "or"))
                return Token { Token::Type::Keyword, GC_STRDUP("or"), m_token_line, m_token_column };
            else if (match(4, "redo"))
                return Token { Token::Type::Keyword, GC_STRDUP("redo"), m_token_line, m_token_column };
            else if (match(6, "rescue"))
                return Token { Token::Type::Keyword, GC_STRDUP("rescue"), m_token_line, m_token_column };
            else if (match(5, "retry"))
                return Token { Token::Type::Keyword, GC_STRDUP("retry"), m_token_line, m_token_column };
            else if (match(6, "return"))
                return Token { Token::Type::Keyword, GC_STRDUP("return"), m_token_line, m_token_column };
            else if (match(4, "self"))
                return Token { Token::Type::Keyword, GC_STRDUP("self"), m_token_line, m_token_column };
            else if (match(5, "super"))
                return Token { Token::Type::Keyword, GC_STRDUP("super"), m_token_line, m_token_column };
            else if (match(4, "then"))
                return Token { Token::Type::Keyword, GC_STRDUP("then"), m_token_line, m_token_column };
            else if (match(4, "true"))
                return Token { Token::Type::Keyword, GC_STRDUP("true"), m_token_line, m_token_column };
            else if (match(5, "undef"))
                return Token { Token::Type::Keyword, GC_STRDUP("undef"), m_token_line, m_token_column };
            else if (match(6, "unless"))
                return Token { Token::Type::Keyword, GC_STRDUP("unless"), m_token_line, m_token_column };
            else if (match(5, "until"))
                return Token { Token::Type::Keyword, GC_STRDUP("until"), m_token_line, m_token_column };
            else if (match(4, "when"))
                return Token { Token::Type::Keyword, GC_STRDUP("when"), m_token_line, m_token_column };
            else if (match(5, "while"))
                return Token { Token::Type::Keyword, GC_STRDUP("while"), m_token_line, m_token_column };
            else if (match(5, "yield"))
                return Token { Token::Type::Keyword, GC_STRDUP("yield"), m_token_line, m_token_column };
            else {
                auto c = current_char();
                bool symbol_key = false;
                if ((c >= 'a' && c <= 'z') || c == '_') {
                    return consume_identifier();
                } else if (c >= 'A' && c <= 'Z') {
                    return consume_constant();
                } else {
                    const char *buf = consume_non_whitespace();
                    return Token { Token::Type::Invalid, buf, m_token_line, m_token_column };
                }
            }
        }
        NAT_UNREACHABLE();
    }

    Token consume_word(Token::Type type) {
        char c = current_char();
        auto buf = std::string("");
        do {
            buf += c;
            advance();
            c = current_char();
        } while (is_identifier_char(c));
        switch (c) {
        case '?':
        case '!':
        case '=':
            if (m_last_token.can_precede_method_name()) {
                advance();
                buf += c;
            } else {
                break;
            }
        default:
            break;
        }
        return Token { type, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
    }

    Token consume_identifier() {
        Token token = consume_word(Token::Type::Identifier);
        auto c = current_char();
        if (c == ':' && peek() != ':') {
            advance();
            token.set_type(Token::Type::SymbolKey);
        }
        return token;
    }

    Token consume_constant() {
        Token token = consume_word(Token::Type::Constant);
        auto c = current_char();
        if (c == ':' && peek() != ':') {
            advance();
            token.set_type(Token::Type::SymbolKey);
        }
        return token;
    }

    Token consume_integer(bool negative) {
        nat_int_t number = 0;
        do {
            number *= 10;
            number += current_char() - 48;
            advance();
        } while (isdigit(current_char()));
        // TODO: check if invalid character follows, such as a letter
        if (negative)
            number *= -1;
        return Token { Token::Type::Integer, number, m_token_line, m_token_column };
    }

    Token consume_double_quoted_string(char delimiter) {
        auto buf = std::string("");
        char c = current_char();
        for (;;) {
            if (!c) return Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            if (c == delimiter) {
                advance();
                return Token { Token::Type::String, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            }
            buf += c;
            advance();
            c = current_char();
        }
        NAT_UNREACHABLE();
    }

    Token consume_single_quoted_string(char delimiter) {
        auto buf = std::string("");
        char c = current_char();
        for (;;) {
            if (!c) return Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            if (c == delimiter) {
                advance();
                return Token { Token::Type::String, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            }
            buf += c;
            advance();
            c = current_char();
        }
        NAT_UNREACHABLE();
    }

    Token consume_quoted_array_without_interpolation(char delimiter, Token::Type type) {
        auto buf = std::string("");
        char c = current_char();
        for (;;) {
            if (!c) return Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            if (c == delimiter) {
                advance();
                return Token { type, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            }
            buf += c;
            advance();
            c = current_char();
        }
        NAT_UNREACHABLE();
    }

    Token consume_quoted_array_with_interpolation(char delimiter, Token::Type type) {
        auto buf = std::string("");
        char c = current_char();
        for (;;) {
            if (!c) return Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            if (c == delimiter) {
                advance();
                return Token { type, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            }
            buf += c;
            advance();
            c = current_char();
        }
        NAT_UNREACHABLE();
    }

    Token consume_regexp() {
        auto buf = std::string("");
        char c = current_char();
        for (;;) {
            if (!c) return Token { Token::Type::UnterminatedRegexp, m_token_line, m_token_column };
            if (c == '/') {
                advance();
                return Token { Token::Type::Regexp, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            }
            buf += c;
            advance();
            c = current_char();
        }
        NAT_UNREACHABLE();
    }

    const char *consume_non_whitespace() {
        char c = current_char();
        auto buf = std::string("");
        do {
            buf += c;
            advance();
            c = current_char();
        } while (c && c != ' ' && c != '\t' && c != '\n' && c != '\r');
        return GC_STRDUP(buf.c_str());
    }

    const char *m_input { nullptr };
    size_t m_size { 0 };
    size_t m_index { 0 };

    // current character position
    size_t m_cursor_line { 0 };
    size_t m_cursor_column { 0 };

    // start of current token
    size_t m_token_line { 0 };
    size_t m_token_column { 0 };

    // if the current token is preceded by whitespace
    bool m_whitespace_precedes { false };

    // the previously-matched token
    Token m_last_token {};
};
}
