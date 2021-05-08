#pragma once

#include <string>

#include "natalie/gc.hpp"
#include "natalie/token.hpp"

namespace Natalie {

struct Lexer : public Cell {
    Lexer(const char *input, const char *file)
        : m_input { input }
        , m_file { GC_STRDUP(file) }
        , m_size { strlen(input) } {
        assert(m_input);
    }

    Vector<Token *> *tokens();

    Token *next_token() {
        m_whitespace_precedes = skip_whitespace();
        m_token_line = m_cursor_line;
        m_token_column = m_cursor_column;
        auto token = build_next_token();
        m_last_token = token;
        return token;
    }

private:
    char current_char() {
        if (m_index >= m_size)
            return 0;
        return m_input[m_index];
    }

    bool match(size_t bytes, const char *compare) {
        if (m_index + bytes > m_size)
            return false;
        if (strncmp(compare, m_input + m_index, bytes) == 0) {
            if (m_index + bytes < m_size && is_identifier_char_or_message_suffix(m_input[m_index + bytes]))
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

    // NOTE: this does not work across lines
    void rewind(size_t bytes = 1) {
        auto c = current_char();
        m_cursor_column -= bytes;
        m_index -= bytes;
    }

    char peek() {
        if (m_index + 1 >= m_size)
            return 0;
        return m_input[m_index + 1];
    }

    bool is_identifier_char(char c) {
        if (!c) return false;
        return isalnum(c) || c == '_';
    }

    bool is_message_suffix(char c) {
        if (!c) return false;
        return c == '?' || c == '!';
    }

    bool is_identifier_char_or_message_suffix(char c) {
        return is_identifier_char(c) || is_message_suffix(c);
    }

    bool skip_whitespace() {
        bool whitespace_found = false;
        while (current_char() == ' ' || current_char() == '\t') {
            whitespace_found = true;
            advance();
        }
        return whitespace_found;
    }

    Token *build_next_token() {
        if (m_index >= m_size)
            return new Token { Token::Type::Eof, m_file, m_token_line, m_token_column };
        Token *token = nullptr;
        switch (current_char()) {
        case '=': {
            advance();
            switch (current_char()) {
            case '=': {
                advance();
                switch (current_char()) {
                case '=': {
                    advance();
                    return new Token { Token::Type::EqualEqualEqual, m_file, m_token_line, m_token_column };
                }
                default:
                    return new Token { Token::Type::EqualEqual, m_file, m_token_line, m_token_column };
                }
            }
            case '>':
                advance();
                return new Token { Token::Type::HashRocket, m_file, m_token_line, m_token_column };
            case '~':
                advance();
                return new Token { Token::Type::Match, m_file, m_token_line, m_token_column };
            default:
                return new Token { Token::Type::Equal, m_file, m_token_line, m_token_column };
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
                    return new Token { Token::Type::Invalid, current_char(), m_file, m_cursor_line, m_cursor_column };
                }
                token->set_has_sign(true);
                return token;
            }
            case '=':
                advance();
                return new Token { Token::Type::PlusEqual, m_file, m_token_line, m_token_column };
            default:
                return new Token { Token::Type::Plus, m_file, m_token_line, m_token_column };
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
                    return new Token { Token::Type::Invalid, current_char(), m_file, m_cursor_line, m_cursor_column };
                }
                token->set_has_sign(true);
                return token;
            }
            case '>':
                advance();
                return new Token { Token::Type::Arrow, m_file, m_token_line, m_token_column };
            default:
                switch (current_char()) {
                case '=':
                    advance();
                    return new Token { Token::Type::MinusEqual, m_file, m_token_line, m_token_column };
                default:
                    return new Token { Token::Type::Minus, m_file, m_token_line, m_token_column };
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
                    return new Token { Token::Type::ExponentEqual, m_file, m_token_line, m_token_column };
                default:
                    return new Token { Token::Type::Exponent, m_file, m_token_line, m_token_column };
                }
            case '=':
                advance();
                return new Token { Token::Type::MultiplyEqual, m_file, m_token_line, m_token_column };
            default:
                return new Token { Token::Type::Multiply, m_file, m_token_line, m_token_column };
            }
        case '/': {
            advance();
            if (!m_last_token)
                return consume_regexp('/');
            switch (m_last_token->type()) {
            case Token::Type::Comma:
            case Token::Type::LCurlyBrace:
            case Token::Type::LBracket:
            case Token::Type::LParen:
            case Token::Type::Match:
                return consume_regexp('/');
            default: {
                switch (current_char()) {
                case ' ':
                    return new Token { Token::Type::Divide, m_file, m_token_line, m_token_column };
                case '=':
                    advance();
                    return new Token { Token::Type::DivideEqual, m_file, m_token_line, m_token_column };
                default:
                    if (m_whitespace_precedes) {
                        return consume_regexp('/');
                    } else {
                        return new Token { Token::Type::Divide, m_file, m_token_line, m_token_column };
                    }
                }
            }
            }
        }
        case '%':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return new Token { Token::Type::ModulusEqual, m_file, m_token_line, m_token_column };
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
            case 'q':
                switch (peek()) {
                case '/':
                    advance(2);
                    return consume_single_quoted_string('/');
                case '[':
                    advance(2);
                    return consume_single_quoted_string(']');
                case '{':
                    advance(2);
                    return consume_single_quoted_string('}');
                case '(':
                    advance(2);
                    return consume_single_quoted_string(')');
                default:
                    return new Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
                }
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
                    return new Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
                }
            case 'x':
                switch (peek()) {
                case '/': {
                    advance(2);
                    auto token = consume_double_quoted_string('/');
                    token->set_type(Token::Type::Shell);
                    return token;
                }
                case '[': {
                    advance(2);
                    auto token = consume_double_quoted_string(']');
                    token->set_type(Token::Type::Shell);
                    return token;
                }
                case '{': {
                    advance(2);
                    auto token = consume_double_quoted_string('}');
                    token->set_type(Token::Type::Shell);
                    return token;
                }
                case '(': {
                    advance(2);
                    auto token = consume_double_quoted_string(')');
                    token->set_type(Token::Type::Shell);
                    return token;
                }
                default:
                    return new Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
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
                    return new Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
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
                    return new Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
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
                    return new Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
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
                    return new Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
                }
            default:
                return new Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case '!':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return new Token { Token::Type::NotEqual, m_file, m_token_line, m_token_column };
            case '~':
                advance();
                return new Token { Token::Type::NotMatch, m_file, m_token_line, m_token_column };
            default:
                return new Token { Token::Type::Not, m_file, m_token_line, m_token_column };
            }
        case '<':
            advance();
            switch (current_char()) {
            case '<': {
                advance();
                switch (current_char()) {
                case '-': {
                    auto next = peek();
                    if (isalpha(next) || next == '_') {
                        return consume_heredoc();
                    } else {
                        return new Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column };
                    }
                }
                case '=':
                    advance();
                    return new Token { Token::Type::LeftShiftEqual, m_file, m_token_line, m_token_column };
                default:
                    if (isalpha(current_char()) || current_char() == '_') {
                        return consume_heredoc();
                    } else {
                        return new Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column };
                    }
                }
            }
            case '=':
                advance();
                switch (current_char()) {
                case '>':
                    advance();
                    return new Token { Token::Type::Comparison, m_file, m_token_line, m_token_column };
                default:
                    return new Token { Token::Type::LessThanOrEqual, m_file, m_token_line, m_token_column };
                }
            default:
                return new Token { Token::Type::LessThan, m_file, m_token_line, m_token_column };
            }
        case '>':
            advance();
            switch (current_char()) {
            case '>':
                advance();
                switch (current_char()) {
                case '=':
                    advance();
                    return new Token { Token::Type::RightShiftEqual, m_file, m_token_line, m_token_column };
                default:
                    return new Token { Token::Type::RightShift, m_file, m_token_line, m_token_column };
                }
            case '=':
                advance();
                return new Token { Token::Type::GreaterThanOrEqual, m_file, m_token_line, m_token_column };
            default:
                return new Token { Token::Type::GreaterThan, m_file, m_token_line, m_token_column };
            }
        case '&':
            advance();
            switch (current_char()) {
            case '&':
                advance();
                switch (current_char()) {
                case '=':
                    advance();
                    return new Token { Token::Type::AndEqual, m_file, m_token_line, m_token_column };
                default:
                    return new Token { Token::Type::And, m_file, m_token_line, m_token_column };
                }
            case '=':
                advance();
                return new Token { Token::Type::BitwiseAndEqual, m_file, m_token_line, m_token_column };
            case '.':
                advance();
                return new Token { Token::Type::SafeNavigation, m_file, m_token_line, m_token_column };
            default:
                return new Token { Token::Type::BitwiseAnd, m_file, m_token_line, m_token_column };
            }
        case '|':
            advance();
            switch (current_char()) {
            case '|':
                advance();
                switch (current_char()) {
                case '=':
                    advance();
                    return new Token { Token::Type::OrEqual, m_file, m_token_line, m_token_column };
                default:
                    return new Token { Token::Type::Or, m_file, m_token_line, m_token_column };
                }
            case '=':
                advance();
                return new Token { Token::Type::BitwiseOrEqual, m_file, m_token_line, m_token_column };
            default:
                return new Token { Token::Type::BitwiseOr, m_file, m_token_line, m_token_column };
            }
        case '^':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return new Token { Token::Type::BitwiseXorEqual, m_file, m_token_line, m_token_column };
            default:
                return new Token { Token::Type::BitwiseXor, m_file, m_token_line, m_token_column };
            }
        case '~':
            advance();
            return new Token { Token::Type::BinaryOnesComplement, m_file, m_token_line, m_token_column };
        case '?':
            advance();
            return new Token { Token::Type::TernaryQuestion, m_file, m_token_line, m_token_column };
        case ':': {
            advance();
            auto c = current_char();
            if (c == ':') {
                advance();
                return new Token { Token::Type::ConstantResolution, m_file, m_token_line, m_token_column };
            } else if (c == '"') {
                advance();
                auto string = consume_double_quoted_string('"');
                return new Token { Token::Type::Symbol, string->literal(), m_file, m_token_line, m_token_column };
            } else if (c == '\'') {
                advance();
                auto string = consume_single_quoted_string('\'');
                return new Token { Token::Type::Symbol, string->literal(), m_file, m_token_line, m_token_column };
            } else if (isspace(c)) {
                return new Token { Token::Type::TernaryColon, m_file, m_token_line, m_token_column };
            } else {
                return consume_symbol();
            }
        }
        case '@':
            switch (peek()) {
            case '@': {
                // kinda janky, but we gotta trick consume_word and then prepend the '@' back on the front
                advance();
                auto token = consume_word(Token::Type::ClassVariable);
                token->set_literal(std::string(1, '@') + std::string(token->literal()));
                return token;
            }
            default:
                return consume_word(Token::Type::InstanceVariable);
            }
        case '$':
            return consume_global_variable();
        case '.':
            advance();
            switch (current_char()) {
            case '.':
                advance();
                switch (current_char()) {
                case '.':
                    advance();
                    return new Token { Token::Type::DotDotDot, m_file, m_token_line, m_token_column };
                default:
                    return new Token { Token::Type::DotDot, m_file, m_token_line, m_token_column };
                }
            default:
                return new Token { Token::Type::Dot, m_file, m_token_line, m_token_column };
            }
        case '{':
            advance();
            return new Token { Token::Type::LCurlyBrace, m_file, m_token_line, m_token_column };
        case '[': {
            advance();
            auto token = new Token { Token::Type::LBracket, m_file, m_token_line, m_token_column };
            token->set_whitespace_precedes(m_whitespace_precedes);
            return token;
        }
        case '(':
            advance();
            return new Token { Token::Type::LParen, m_file, m_token_line, m_token_column };
        case '}':
            advance();
            return new Token { Token::Type::RCurlyBrace, m_file, m_token_line, m_token_column };
        case ']':
            advance();
            return new Token { Token::Type::RBracket, m_file, m_token_line, m_token_column };
        case ')':
            advance();
            return new Token { Token::Type::RParen, m_file, m_token_line, m_token_column };
        case '\n': {
            advance();
            auto token = new Token { Token::Type::Eol, m_file, m_token_line, m_token_column };
            while (m_index < m_index_after_heredoc)
                advance();
            return token;
        }
        case ';':
            advance();
            return new Token { Token::Type::Semicolon, m_file, m_token_line, m_token_column };
        case ',':
            advance();
            return new Token { Token::Type::Comma, m_file, m_token_line, m_token_column };
        case '"':
            advance();
            return consume_double_quoted_string('"');
        case '\'':
            advance();
            return consume_single_quoted_string('\'');
        case '`': {
            advance();
            auto token = consume_double_quoted_string('`');
            token->set_type(Token::Type::Shell);
            return token;
        }
        case '#':
            char c;
            do {
                advance();
                c = current_char();
            } while (c && c != '\n' && c != '\r');
            return new Token { Token::Type::Comment, m_file, m_token_line, m_token_column };
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
                return new Token { Token::Type::Invalid, current_char(), m_file, m_cursor_line, m_cursor_column };
            }
            return token;
        }
        };
        if (!token) {
            if (match(12, "__ENCODING__"))
                return new Token { Token::Type::ENCODINGKeyword, m_file, m_token_line, m_token_column };
            else if (match(8, "__LINE__"))
                return new Token { Token::Type::LINEKeyword, m_file, m_token_line, m_token_column };
            else if (match(8, "__FILE__"))
                return new Token { Token::Type::FILEKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "BEGIN"))
                return new Token { Token::Type::BEGINKeyword, m_file, m_token_line, m_token_column };
            else if (match(3, "END"))
                return new Token { Token::Type::ENDKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "alias"))
                return new Token { Token::Type::AliasKeyword, m_file, m_token_line, m_token_column };
            else if (match(3, "and"))
                return new Token { Token::Type::AndKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "begin"))
                return new Token { Token::Type::BeginKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "break"))
                return new Token { Token::Type::BreakKeyword, m_file, m_token_line, m_token_column };
            else if (match(4, "case"))
                return new Token { Token::Type::CaseKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "class"))
                return new Token { Token::Type::ClassKeyword, m_file, m_token_line, m_token_column };
            else if (match(8, "defined?"))
                return new Token { Token::Type::DefinedKeyword, m_file, m_token_line, m_token_column };
            else if (match(3, "def"))
                return new Token { Token::Type::DefKeyword, m_file, m_token_line, m_token_column };
            else if (match(2, "do"))
                return new Token { Token::Type::DoKeyword, m_file, m_token_line, m_token_column };
            else if (match(4, "else"))
                return new Token { Token::Type::ElseKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "elsif"))
                return new Token { Token::Type::ElsifKeyword, m_file, m_token_line, m_token_column };
            else if (match(3, "end"))
                return new Token { Token::Type::EndKeyword, m_file, m_token_line, m_token_column };
            else if (match(6, "ensure"))
                return new Token { Token::Type::EnsureKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "false"))
                return new Token { Token::Type::FalseKeyword, m_file, m_token_line, m_token_column };
            else if (match(3, "for"))
                return new Token { Token::Type::ForKeyword, m_file, m_token_line, m_token_column };
            else if (match(2, "if"))
                return new Token { Token::Type::IfKeyword, m_file, m_token_line, m_token_column };
            else if (match(2, "in"))
                return new Token { Token::Type::InKeyword, m_file, m_token_line, m_token_column };
            else if (match(6, "module"))
                return new Token { Token::Type::ModuleKeyword, m_file, m_token_line, m_token_column };
            else if (match(4, "next"))
                return new Token { Token::Type::NextKeyword, m_file, m_token_line, m_token_column };
            else if (match(3, "nil"))
                return new Token { Token::Type::NilKeyword, m_file, m_token_line, m_token_column };
            else if (match(3, "not"))
                return new Token { Token::Type::NotKeyword, m_file, m_token_line, m_token_column };
            else if (match(2, "or"))
                return new Token { Token::Type::OrKeyword, m_file, m_token_line, m_token_column };
            else if (match(4, "redo"))
                return new Token { Token::Type::RedoKeyword, m_file, m_token_line, m_token_column };
            else if (match(6, "rescue"))
                return new Token { Token::Type::RescueKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "retry"))
                return new Token { Token::Type::RetryKeyword, m_file, m_token_line, m_token_column };
            else if (match(6, "return"))
                return new Token { Token::Type::ReturnKeyword, m_file, m_token_line, m_token_column };
            else if (match(4, "self"))
                return new Token { Token::Type::SelfKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "super"))
                return new Token { Token::Type::SuperKeyword, m_file, m_token_line, m_token_column };
            else if (match(4, "then"))
                return new Token { Token::Type::ThenKeyword, m_file, m_token_line, m_token_column };
            else if (match(4, "true"))
                return new Token { Token::Type::TrueKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "undef"))
                return new Token { Token::Type::UndefKeyword, m_file, m_token_line, m_token_column };
            else if (match(6, "unless"))
                return new Token { Token::Type::UnlessKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "until"))
                return new Token { Token::Type::UntilKeyword, m_file, m_token_line, m_token_column };
            else if (match(4, "when"))
                return new Token { Token::Type::WhenKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "while"))
                return new Token { Token::Type::WhileKeyword, m_file, m_token_line, m_token_column };
            else if (match(5, "yield"))
                return new Token { Token::Type::YieldKeyword, m_file, m_token_line, m_token_column };
            auto c = current_char();
            bool symbol_key = false;
            if ((c >= 'a' && c <= 'z') || c == '_') {
                return consume_bare_name();
            } else if (c >= 'A' && c <= 'Z') {
                return consume_constant();
            } else {
                const char *buf = consume_non_whitespace();
                return new Token { Token::Type::Invalid, buf, m_file, m_token_line, m_token_column };
            }
        }
        NAT_UNREACHABLE();
    }

    Token *consume_symbol() {
        char c = current_char();
        auto buf = std::string("");
        auto gobble = [&buf, this](char c) -> char { buf += c; advance(); return current_char(); };
        switch (c) {
        case '@':
            c = gobble(c);
            if (c == '@') c = gobble(c);
            do {
                c = gobble(c);
            } while (is_identifier_char(c));
            break;
        case '$':
            c = gobble(c);
            do {
                c = gobble(c);
            } while (is_identifier_char(c));
            break;
        case '+':
        case '-':
        case '/':
            c = gobble(c);
            break;
        case '*':
            c = gobble(c);
            if (c == '*') c = gobble(c);
            break;
        case '=':
            c = gobble(c);
            if (c == '=') c = gobble(c);
            break;
        case '[':
            if (peek() == ']') {
                c = gobble(c);
                c = gobble(c);
                if (c == '=') c = gobble(c);
            } else {
                return new Token { Token::Type::TernaryColon, m_file, m_token_line, m_token_column };
            }
            break;
        default:
            do {
                c = gobble(c);
            } while (is_identifier_char(c));
            switch (c) {
            case '?':
            case '!':
            case '=':
                gobble(c);
            default:
                break;
            }
        }
        return new Token { Token::Type::Symbol, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
    }

    Token *consume_word(Token::Type type) {
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
            advance();
            buf += c;
            break;
        default:
            break;
        }
        return new Token { type, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
    }

    Token *consume_bare_name() {
        Token *token = consume_word(Token::Type::BareName);
        auto c = current_char();
        if (c == ':' && peek() != ':') {
            advance();
            token->set_type(Token::Type::SymbolKey);
        }
        return token;
    }

    Token *consume_constant() {
        Token *token = consume_word(Token::Type::Constant);
        auto c = current_char();
        if (c == ':' && peek() != ':') {
            advance();
            token->set_type(Token::Type::SymbolKey);
        }
        return token;
    }

    Token *consume_global_variable() {
        switch (peek()) {
        case '?':
        case '!':
        case '=': {
            advance();
            auto buf = std::string("$") + current_char();
            advance();
            return new Token { Token::Type::GlobalVariable, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
        }
        default: {
            return consume_word(Token::Type::GlobalVariable);
        }
        }
    }

    Token *consume_heredoc() {
        bool with_dash = false;
        if (current_char() == '-') {
            advance();
            with_dash = true;
        }
        auto heredoc_name = consume_word(Token::Type::BareName);
        auto doc = std::string();
        size_t heredoc_index = m_index;
        auto get_char = [&heredoc_index, this]() { return (heredoc_index >= m_size) ? 0 : m_input[heredoc_index]; };

        // start consuming the heredoc on the next line
        while (get_char() != '\n') {
            if (heredoc_index >= m_size)
                return new Token { Token::Type::UnterminatedString, GC_STRDUP("heredoc"), m_file, m_token_line, m_token_column };
            heredoc_index++;
        }
        heredoc_index++;

        // consume the heredoc until we find the delimiter, either "\nDELIM\n" (if << was used) or "DELIM\n" (if <<- was used)
        auto delimiter = std::string(heredoc_name->literal()) + '\n';
        if (!with_dash)
            delimiter = '\n' + delimiter;
        while (doc.find(delimiter) == std::string::npos) {
            if (heredoc_index >= m_size)
                return new Token { Token::Type::UnterminatedString, GC_STRDUP(doc.c_str()), m_file, m_token_line, m_token_column };
            doc += get_char();
            heredoc_index++;
        }

        // chop the delimiter and any trailing space off the string
        doc.resize(doc.size() - delimiter.size() + (with_dash ? 0 : 1));
        while (doc.size() > 0 && doc[doc.size() - 1] != '\n')
            doc.resize(doc.size() - 1);

        // we have to keep tokenizing on the line where the heredoc was started, and then jump to the line after the heredoc
        // this index is used to do that
        m_index_after_heredoc = heredoc_index;

        auto token = new Token { Token::Type::DoubleQuotedString, GC_STRDUP(doc.c_str()), m_file, m_token_line, m_token_column };
        return token;
    }

    Token *consume_numeric(bool negative = false) {
        size_t start_index = m_index;
        nat_int_t number = 0;
        if (current_char() == '0') {
            switch (peek()) {
            case 'd':
            case 'D': {
                advance(2);
                char c = current_char();
                if (!isdigit(c))
                    return new Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
                do {
                    number *= 10;
                    number += c - 48;
                    advance();
                    c = current_char();
                    if (c == '_') {
                        advance();
                        c = current_char();
                    }
                } while (isdigit(c));
                if (negative)
                    number *= -1;
                return new Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
            }
            case 'o':
            case 'O': {
                advance(2);
                char c = current_char();
                if (!(c >= '0' && c <= '7'))
                    return new Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
                do {
                    number *= 8;
                    number += c - 48;
                    advance();
                    c = current_char();
                    if (c == '_') {
                        advance();
                        c = current_char();
                    }
                } while (c >= '0' && c <= '7');
                if (negative)
                    number *= -1;
                return new Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
            }
            case 'x':
            case 'X': {
                advance(2);
                char c = current_char();
                if (!isxdigit(c))
                    return new Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
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
                    if (c == '_') {
                        advance();
                        c = current_char();
                    }
                } while (isxdigit(c));
                if (negative)
                    number *= -1;
                return new Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
            }
            case 'b':
            case 'B': {
                advance(2);
                char c = current_char();
                if (c != '0' && c != '1')
                    return new Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
                do {
                    number *= 2;
                    number += c - 48;
                    advance();
                    c = current_char();
                    if (c == '_') {
                        advance();
                        c = current_char();
                    }
                } while (c == '0' || c == '1');
                if (negative)
                    number *= -1;
                return new Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
            }
            }
        }
        char c = current_char();
        do {
            number *= 10;
            number += c - 48;
            advance();
            c = current_char();
            if (c == '_') {
                advance();
                c = current_char();
            }
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
            return new Token { Token::Type::Float, dbl, m_file, m_token_line, m_token_column };
        } else {
            if (negative)
                number *= -1;
            return new Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
        }
    }

    Token *consume_double_quoted_string(char delimiter) {
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
                auto token = new Token { Token::Type::DoubleQuotedString, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
                return token;
            } else {
                buf += c;
            }
            advance();
            c = current_char();
        }
        return new Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
    }

    Token *consume_single_quoted_string(char delimiter) {
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
                return new Token { Token::Type::String, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
            } else {
                buf += c;
            }
            advance();
            c = current_char();
        }
        return new Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
    }

    Token *consume_quoted_array_without_interpolation(char delimiter, Token::Type type) {
        auto buf = std::string("");
        char c = current_char();
        bool seen_space = false;
        bool seen_start = false;
        for (;;) {
            if (!c) return new Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
            if (c == delimiter) {
                advance();
                break;
            }
            switch (c) {
            case ' ':
            case '\t':
            case '\n':
                if (!seen_space && seen_start) buf += ' ';
                seen_space = true;
                break;
            default:
                buf += c;
                seen_space = false;
                seen_start = true;
            }
            advance();
            c = current_char();
        }
        if (buf[buf.length() - 1] == ' ') buf.erase(buf.length() - 1, 1);
        return new Token { type, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
    }

    Token *consume_quoted_array_with_interpolation(char delimiter, Token::Type type) {
        auto buf = std::string("");
        char c = current_char();
        bool seen_space = false;
        bool seen_start = false;
        for (;;) {
            if (!c) return new Token { Token::Type::UnterminatedString, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
            if (c == delimiter) {
                advance();
                break;
            }
            switch (c) {
            case ' ':
            case '\t':
            case '\n':
                if (!seen_space && seen_start) buf += ' ';
                seen_space = true;
                break;
            default:
                buf += c;
                seen_space = false;
                seen_start = true;
            }
            advance();
            c = current_char();
        }
        if (buf[buf.length() - 1] == ' ') buf.erase(buf.length() - 1, 1);
        return new Token { type, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
    }

    Token *consume_regexp(char delimiter) {
        auto buf = std::string("");
        char c = current_char();
        while (c) {
            if (c == '\\') {
                buf += c;
                advance();
                buf += current_char();
            } else if (c == delimiter) {
                advance();
                auto token = new Token { Token::Type::Regexp, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
                return token;
            } else {
                buf += c;
            }
            advance();
            c = current_char();
        }
        return new Token { Token::Type::UnterminatedRegexp, GC_STRDUP(buf.c_str()), m_file, m_token_line, m_token_column };
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
    const char *m_file { nullptr };
    size_t m_size { 0 };
    size_t m_index { 0 };

    // if non-zero, the index we should continue on after a linebreak
    size_t m_index_after_heredoc { 0 };

    // current character position
    size_t m_cursor_line { 0 };
    size_t m_cursor_column { 0 };

    // start of current token
    size_t m_token_line { 0 };
    size_t m_token_column { 0 };

    // if the current token is preceded by whitespace
    bool m_whitespace_precedes { false };

    // the previously-matched token
    Token *m_last_token { nullptr };

    virtual void visit_children(Visitor &) override final {
    }
};

struct InterpolatedStringLexer {
    InterpolatedStringLexer(Token *token)
        : m_input { token->literal() }
        , m_file { token->file() }
        , m_line { token->line() }
        , m_column { token->column() }
        , m_size { strlen(token->literal()) } { }

    Vector<Token *> *tokens() {
        auto tokens = new Vector<Token *> {};
        auto raw = std::string("");
        while (m_index < m_size) {
            char c = current_char();
            if (c == '#' && peek() == '{') {
                if (!raw.empty() || tokens->is_empty()) {
                    tokens->push(new Token { Token::Type::String, GC_STRDUP(raw.c_str()), m_file, m_line, m_column });
                    raw.clear();
                }
                m_index += 2;
                tokenize_interpolation(tokens);
            } else {
                raw += c;
                m_index++;
            }
        }
        if (!raw.empty())
            tokens->push(new Token { Token::Type::String, GC_STRDUP(raw.c_str()), m_file, m_line, m_column });
        return tokens;
    }

private:
    void tokenize_interpolation(Vector<Token *> *);

    char current_char() {
        if (m_index >= m_size)
            return 0;
        return m_input[m_index];
    }

    char peek() {
        if (m_index + 1 >= m_size)
            return 0;
        return m_input[m_index + 1];
    }

    const char *m_input { nullptr };
    const char *m_file { nullptr };
    size_t m_line { 0 };
    size_t m_column { 0 };
    size_t m_size { 0 };
    size_t m_index { 0 };
};

}
