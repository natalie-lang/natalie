#pragma once

#include <string>

#include "natalie.hpp"
#include "natalie/token.hpp"

namespace Natalie {

struct Lexer : public gc {
    Lexer(const char *input)
        : m_input { input }
        , m_size { strlen(input) } {
        assert(m_input);
    }

    Vector<Token> *tokens() {
        auto tokens = new Vector<Token> {};
        bool skip_next_newline = false;
        for (;;) {
            auto token = next_token();
            if (token.is_comment())
                continue;

            // get rid of newlines after certain tokens
            if (skip_next_newline && token.is_newline())
                continue;
            if (skip_next_newline && !token.is_newline())
                skip_next_newline = false;

            // get rid of newlines before certain tokens
            while (token.can_follow_collapsible_newline() && !tokens->is_empty() && tokens->last().is_newline())
                tokens->pop();

            // convert semicolons to eol tokens
            if (token.is_semicolon())
                token = Token { Token::Type::Eol, token.line(), token.column() };

            tokens->push(token);

            if (token.is_eof())
                return tokens;
            if (!token.is_valid())
                return tokens;
            if (token.can_precede_collapsible_newline())
                skip_next_newline = true;
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
                auto token = consume_numeric();
                if (isalpha(current_char())) {
                    return Token { Token::Type::Invalid, current_char(), m_cursor_line, m_cursor_column };
                }
                token.set_has_sign(true);
                return token;
            }
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
                auto token = consume_numeric(is_negative);
                if (isalpha(current_char())) {
                    return Token { Token::Type::Invalid, current_char(), m_cursor_line, m_cursor_column };
                }
                token.set_has_sign(true);
                return token;
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
            case '.':
                advance();
                return Token { Token::Type::SafeNavigation, m_token_line, m_token_column };
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
            auto token = consume_numeric();
            if (isalpha(current_char())) {
                return Token { Token::Type::Invalid, current_char(), m_cursor_line, m_cursor_column };
            }
            return token;
        };
        if (!token.is_valid()) {
            if (match(12, "__ENCODING__"))
                return Token { Token::Type::ENCODINGKeyword, m_token_line, m_token_column };
            else if (match(8, "__LINE__"))
                return Token { Token::Type::LINEKeyword, m_token_line, m_token_column };
            else if (match(8, "__FILE__"))
                return Token { Token::Type::FILEKeyword, m_token_line, m_token_column };
            else if (match(5, "BEGIN"))
                return Token { Token::Type::BEGINKeyword, m_token_line, m_token_column };
            else if (match(3, "END"))
                return Token { Token::Type::ENDKeyword, m_token_line, m_token_column };
            else if (match(5, "alias"))
                return Token { Token::Type::AliasKeyword, m_token_line, m_token_column };
            else if (match(3, "and"))
                return Token { Token::Type::AndKeyword, m_token_line, m_token_column };
            else if (match(5, "begin"))
                return Token { Token::Type::BeginKeyword, m_token_line, m_token_column };
            else if (match(5, "break"))
                return Token { Token::Type::BreakKeyword, m_token_line, m_token_column };
            else if (match(4, "case"))
                return Token { Token::Type::CaseKeyword, m_token_line, m_token_column };
            else if (match(5, "class"))
                return Token { Token::Type::ClassKeyword, m_token_line, m_token_column };
            else if (match(8, "defined?"))
                return Token { Token::Type::DefinedKeyword, m_token_line, m_token_column };
            else if (match(3, "def"))
                return Token { Token::Type::DefKeyword, m_token_line, m_token_column };
            else if (match(2, "do"))
                return Token { Token::Type::DoKeyword, m_token_line, m_token_column };
            else if (match(4, "else"))
                return Token { Token::Type::ElseKeyword, m_token_line, m_token_column };
            else if (match(5, "elsif"))
                return Token { Token::Type::ElsifKeyword, m_token_line, m_token_column };
            else if (match(3, "end"))
                return Token { Token::Type::EndKeyword, m_token_line, m_token_column };
            else if (match(6, "ensure"))
                return Token { Token::Type::EnsureKeyword, m_token_line, m_token_column };
            else if (match(5, "false"))
                return Token { Token::Type::FalseKeyword, m_token_line, m_token_column };
            else if (match(3, "for"))
                return Token { Token::Type::ForKeyword, m_token_line, m_token_column };
            else if (match(2, "if"))
                return Token { Token::Type::IfKeyword, m_token_line, m_token_column };
            else if (match(2, "in"))
                return Token { Token::Type::InKeyword, m_token_line, m_token_column };
            else if (match(6, "module"))
                return Token { Token::Type::ModuleKeyword, m_token_line, m_token_column };
            else if (match(4, "next"))
                return Token { Token::Type::NextKeyword, m_token_line, m_token_column };
            else if (match(3, "nil"))
                return Token { Token::Type::NilKeyword, m_token_line, m_token_column };
            else if (match(3, "not"))
                return Token { Token::Type::NotKeyword, m_token_line, m_token_column };
            else if (match(2, "or"))
                return Token { Token::Type::OrKeyword, m_token_line, m_token_column };
            else if (match(4, "redo"))
                return Token { Token::Type::RedoKeyword, m_token_line, m_token_column };
            else if (match(6, "rescue"))
                return Token { Token::Type::RescueKeyword, m_token_line, m_token_column };
            else if (match(5, "retry"))
                return Token { Token::Type::RetryKeyword, m_token_line, m_token_column };
            else if (match(6, "return"))
                return Token { Token::Type::ReturnKeyword, m_token_line, m_token_column };
            else if (match(4, "self"))
                return Token { Token::Type::SelfKeyword, m_token_line, m_token_column };
            else if (match(5, "super"))
                return Token { Token::Type::SuperKeyword, m_token_line, m_token_column };
            else if (match(4, "then"))
                return Token { Token::Type::ThenKeyword, m_token_line, m_token_column };
            else if (match(4, "true"))
                return Token { Token::Type::TrueKeyword, m_token_line, m_token_column };
            else if (match(5, "undef"))
                return Token { Token::Type::UndefKeyword, m_token_line, m_token_column };
            else if (match(6, "unless"))
                return Token { Token::Type::UnlessKeyword, m_token_line, m_token_column };
            else if (match(5, "until"))
                return Token { Token::Type::UntilKeyword, m_token_line, m_token_column };
            else if (match(4, "when"))
                return Token { Token::Type::WhenKeyword, m_token_line, m_token_column };
            else if (match(5, "while"))
                return Token { Token::Type::WhileKeyword, m_token_line, m_token_column };
            else if (match(5, "yield"))
                return Token { Token::Type::YieldKeyword, m_token_line, m_token_column };
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

    Token consume_numeric(bool negative = false) {
        size_t start_index = m_index;
        nat_int_t number = 0;
        if (current_char() == '0') {
            switch (peek()) {
            case 'd':
            case 'D': {
                advance(2);
                char c = current_char();
                if (!isdigit(c))
                    return Token { Token::Type::Invalid, c, m_cursor_line, m_cursor_column };
                do {
                    number *= 10;
                    number += c - 48;
                    advance();
                    c = current_char();
                } while (isdigit(c));
                if (negative)
                    number *= -1;
                return Token { Token::Type::Integer, number, m_token_line, m_token_column };
            }
            case 'o':
            case 'O': {
                advance(2);
                char c = current_char();
                if (!(c >= '0' && c <= '7'))
                    return Token { Token::Type::Invalid, c, m_cursor_line, m_token_column };
                do {
                    number *= 8;
                    number += c - 48;
                    advance();
                    c = current_char();
                } while (c >= '0' && c <= '7');
                if (negative)
                    number *= -1;
                return Token { Token::Type::Integer, number, m_token_line, m_token_column };
            }
            case 'x':
            case 'X': {
                advance(2);
                char c = current_char();
                if (!isxdigit(c))
                    return Token { Token::Type::Invalid, c, m_cursor_line, m_cursor_column };
                do {
                    number *= 16;
                    if (c >= 'a' && c <= 'f')
                        number += c - 97 + 10;
                    else if (c >= 'A' && c <= 'F')
                        number += c - 65 + 10;
                    else
                        number += c - 48;
                    advance();
                    c = current_char();
                } while (isxdigit(c));
                if (negative)
                    number *= -1;
                return Token { Token::Type::Integer, number, m_token_line, m_token_column };
            }
            case 'b':
            case 'B': {
                advance(2);
                char c = current_char();
                if (c != '0' && c != '1')
                    return Token { Token::Type::Invalid, c, m_cursor_line, m_cursor_column };
                do {
                    number *= 2;
                    number += c - 48;
                    advance();
                    c = current_char();
                } while (c == '0' || c == '1');
                if (negative)
                    number *= -1;
                return Token { Token::Type::Integer, number, m_token_line, m_token_column };
            }
            }
        }
        char c = current_char();
        do {
            number *= 10;
            number += c - 48;
            advance();
            c = current_char();
        } while (isdigit(c));
        if (c == '.' && isdigit(peek())) {
            advance();
            while (isdigit(current_char())) {
                advance();
            }
            char *endptr = nullptr;
            double dbl = ::strtod(m_input + start_index, &endptr);
            assert(endptr == m_input + m_index); // FIXME: return Invalid token?
            if (negative)
                dbl *= -1;
            return Token { Token::Type::Float, dbl, m_token_line, m_token_column };
        } else {
            if (negative)
                number *= -1;
            return Token { Token::Type::Integer, number, m_token_line, m_token_column };
        }
    }

    Token consume_double_quoted_string(char delimiter) {
        auto buf = std::string("");
        char c = current_char();
        while (c) {
            if (c == '\\') {
                advance();
                c = current_char();
                switch (c) {
                case 'n':
                    buf += '\n';
                    break;
                case 't':
                    buf += '\t';
                    break;
                default:
                    buf += c;
                    break;
                }
            } else if (c == delimiter) {
                advance();
                return Token { Token::Type::String, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            } else {
                buf += c;
            }
            advance();
            c = current_char();
        }
        return Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
    }

    Token consume_single_quoted_string(char delimiter) {
        auto buf = std::string("");
        char c = current_char();
        while (c) {
            if (c == '\\') {
                advance();
                c = current_char();
                switch (c) {
                case '\\':
                case '\'':
                    buf += c;
                    break;
                default:
                    buf += '\\';
                    buf += c;
                    break;
                }
            } else if (c == delimiter) {
                advance();
                return Token { Token::Type::String, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
            } else {
                buf += c;
            }
            advance();
            c = current_char();
        }
        return Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_token_line, m_token_column };
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
