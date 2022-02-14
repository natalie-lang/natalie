#include <limits>

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/token.hpp"

namespace NatalieParser {

SharedPtr<Vector<Token>> Lexer::tokens() {
    SharedPtr<Vector<Token>> tokens = new Vector<Token> {};
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
            token = Token { Token::Type::Eol, token.file(), token.line(), token.column() };

        // break apart interpolations in double-quoted string
        if (token.can_have_interpolation()) {
            Token::Type begin_token_type = Token::Type::InterpolatedStringBegin;
            Token::Type end_token_type = Token::Type::InterpolatedStringEnd;
            if (token.type() == Token::Type::Shell) {
                begin_token_type = Token::Type::InterpolatedShellBegin;
                end_token_type = Token::Type::InterpolatedShellEnd;
            } else if (token.type() == Token::Type::Regexp) {
                begin_token_type = Token::Type::InterpolatedRegexpBegin;
                end_token_type = Token::Type::InterpolatedRegexpEnd;
            }
            auto string_lexer = InterpolatedStringLexer { token };
            tokens->push(Token { begin_token_type, token.file(), token.line(), token.column() });
            auto string_tokens = string_lexer.tokens();
            for (auto token : *string_tokens) {
                tokens->push(token);
            }
            auto end_token = Token { end_token_type, token.file(), token.line(), token.column() };
            if (token.options())
                end_token.set_options(token.options().value());
            tokens->push(end_token);
            continue;
        }

        tokens->push(token);

        if (token.is_eof())
            return tokens;
        if (!token.is_valid())
            return tokens;
        if (token.can_precede_collapsible_newline())
            skip_next_newline = true;
    };
    TM_UNREACHABLE();
}

Token Lexer::next_token() {
    m_whitespace_precedes = skip_whitespace();
    m_token_line = m_cursor_line;
    m_token_column = m_cursor_column;
    auto token = build_next_token();
    m_last_token = token;
    return token;
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

bool Lexer::match(size_t bytes, const char *compare) {
    if (m_index + bytes > m_size)
        return false;
    if (strncmp(compare, m_input->c_str() + m_index, bytes) == 0) {
        if (m_index + bytes < m_size && is_identifier_char_or_message_suffix(m_input->at(m_index + bytes)))
            return false;
        advance(bytes);
        return true;
    }
    return false;
}

void Lexer::advance() {
    auto c = current_char();
    m_index++;
    if (c == '\n') {
        m_cursor_line++;
        m_cursor_column = 0;
    } else {
        m_cursor_column++;
    }
}

void Lexer::advance(size_t bytes) {
    for (size_t i = 0; i < bytes; i++) {
        advance();
    }
}

// NOTE: this does not work across lines
void Lexer::rewind(size_t bytes) {
    current_char();
    m_cursor_column -= bytes;
    m_index -= bytes;
}

bool Lexer::skip_whitespace() {
    bool whitespace_found = false;
    while (current_char() == ' ' || current_char() == '\t') {
        whitespace_found = true;
        advance();
    }
    return whitespace_found;
}

Token Lexer::build_next_token() {
    if (m_index >= m_size)
        return Token { Token::Type::Eof, m_file, m_token_line, m_token_column };
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
                return Token { Token::Type::EqualEqualEqual, m_file, m_token_line, m_token_column };
            }
            default:
                return Token { Token::Type::EqualEqual, m_file, m_token_line, m_token_column };
            }
        }
        case '>':
            advance();
            return Token { Token::Type::HashRocket, m_file, m_token_line, m_token_column };
        case '~':
            advance();
            return Token { Token::Type::Match, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::Equal, m_file, m_token_line, m_token_column };
        }
    }
    case '+':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::PlusEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::Plus, m_file, m_token_line, m_token_column };
        }
    case '-':
        advance();
        switch (current_char()) {
        case '>':
            advance();
            return Token { Token::Type::Arrow, m_file, m_token_line, m_token_column };
        default:
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::MinusEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Minus, m_file, m_token_line, m_token_column };
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
                return Token { Token::Type::ExponentEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Exponent, m_file, m_token_line, m_token_column };
            }
        case '=':
            advance();
            return Token { Token::Type::MultiplyEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::Multiply, m_file, m_token_line, m_token_column };
        }
    case '/': {
        advance();
        if (!m_last_token)
            return consume_regexp('/');
        switch (m_last_token.type()) {
        case Token::Type::Comma:
        case Token::Type::LBracket:
        case Token::Type::LCurlyBrace:
        case Token::Type::LParen:
        case Token::Type::Match:
            return consume_regexp('/');
        case Token::Type::DefKeyword:
            return Token { Token::Type::Divide, m_file, m_token_line, m_token_column };
        default: {
            switch (current_char()) {
            case ' ':
                return Token { Token::Type::Divide, m_file, m_token_line, m_token_column };
            case '=':
                advance();
                return Token { Token::Type::DivideEqual, m_file, m_token_line, m_token_column };
            default:
                if (m_whitespace_precedes) {
                    return consume_regexp('/');
                } else {
                    return Token { Token::Type::Divide, m_file, m_token_line, m_token_column };
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
            return Token { Token::Type::ModulusEqual, m_file, m_token_line, m_token_column };
        case '/':
        case '|': {
            char c = current_char();
            advance();
            return consume_single_quoted_string(c);
        }
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
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
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
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'r':
            switch (peek()) {
            case '/':
                advance(2);
                return consume_regexp('/');
            case '[':
                advance(2);
                return consume_regexp(']');
            case '{':
                advance(2);
                return consume_regexp('}');
            case '(':
                advance(2);
                return consume_regexp(')');
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'x':
            switch (peek()) {
            case '/': {
                advance(2);
                auto token = consume_double_quoted_string('/');
                token.set_type(Token::Type::Shell);
                return token;
            }
            case '[': {
                advance(2);
                auto token = consume_double_quoted_string(']');
                token.set_type(Token::Type::Shell);
                return token;
            }
            case '{': {
                advance(2);
                auto token = consume_double_quoted_string('}');
                token.set_type(Token::Type::Shell);
                return token;
            }
            case '(': {
                advance(2);
                auto token = consume_double_quoted_string(')');
                token.set_type(Token::Type::Shell);
                return token;
            }
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'w':
            switch (peek()) {
            case '/':
            case '|': {
                advance();
                char c = current_char();
                advance();
                return consume_quoted_array_without_interpolation(c, Token::Type::PercentLowerW);
            }
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
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'W':
            switch (peek()) {
            case '/':
            case '|': {
                advance();
                char c = current_char();
                advance();
                return consume_quoted_array_without_interpolation(c, Token::Type::PercentUpperW);
            }
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
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
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
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
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
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        default:
            return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
        }
    case '!':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::NotEqual, m_file, m_token_line, m_token_column };
        case '~':
            advance();
            return Token { Token::Type::NotMatch, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::Not, m_file, m_token_line, m_token_column };
        }
    case '<':
        advance();
        switch (current_char()) {
        case '<': {
            advance();
            switch (current_char()) {
            case '~':
            case '-': {
                auto next = peek();
                if (isalpha(next))
                    return consume_heredoc();
                switch (next) {
                case '_':
                case '"':
                case '`':
                case '\'':
                    return consume_heredoc();
                default:
                    return Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column };
                }
            }
            case '=':
                advance();
                return Token { Token::Type::LeftShiftEqual, m_file, m_token_line, m_token_column };
            default:
                if (isalpha(current_char()))
                    return consume_heredoc();
                switch (current_char()) {
                case '_':
                case '"':
                case '`':
                case '\'':
                    return consume_heredoc();
                default:
                    return Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column };
                }
            }
        }
        case '=':
            advance();
            switch (current_char()) {
            case '>':
                advance();
                return Token { Token::Type::Comparison, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::LessThanOrEqual, m_file, m_token_line, m_token_column };
            }
        default:
            return Token { Token::Type::LessThan, m_file, m_token_line, m_token_column };
        }
    case '>':
        advance();
        switch (current_char()) {
        case '>':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::RightShiftEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::RightShift, m_file, m_token_line, m_token_column };
            }
        case '=':
            advance();
            return Token { Token::Type::GreaterThanOrEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::GreaterThan, m_file, m_token_line, m_token_column };
        }
    case '&':
        advance();
        switch (current_char()) {
        case '&':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::AndEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::And, m_file, m_token_line, m_token_column };
            }
        case '=':
            advance();
            return Token { Token::Type::BitwiseAndEqual, m_file, m_token_line, m_token_column };
        case '.':
            advance();
            return Token { Token::Type::SafeNavigation, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::BitwiseAnd, m_file, m_token_line, m_token_column };
        }
    case '|':
        advance();
        switch (current_char()) {
        case '|':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::OrEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Or, m_file, m_token_line, m_token_column };
            }
        case '=':
            advance();
            return Token { Token::Type::BitwiseOrEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::BitwiseOr, m_file, m_token_line, m_token_column };
        }
    case '^':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::BitwiseXorEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::BitwiseXor, m_file, m_token_line, m_token_column };
        }
    case '~':
        advance();
        return Token { Token::Type::BinaryOnesComplement, m_file, m_token_line, m_token_column };
    case '?':
        advance();
        return Token { Token::Type::TernaryQuestion, m_file, m_token_line, m_token_column };
    case ':': {
        advance();
        auto c = current_char();
        if (c == ':') {
            advance();
            return Token { Token::Type::ConstantResolution, m_file, m_token_line, m_token_column };
        } else if (c == '"') {
            advance();
            auto string = consume_double_quoted_string('"');
            return Token { Token::Type::Symbol, string.literal(), m_file, m_token_line, m_token_column };
        } else if (c == '\'') {
            advance();
            auto string = consume_single_quoted_string('\'');
            return Token { Token::Type::Symbol, string.literal(), m_file, m_token_line, m_token_column };
        } else if (isspace(c)) {
            return Token { Token::Type::TernaryColon, m_file, m_token_line, m_token_column };
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
            token.set_literal(String::format("@{}", token.literal()));
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
                return Token { Token::Type::DotDotDot, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::DotDot, m_file, m_token_line, m_token_column };
            }
        default:
            return Token { Token::Type::Dot, m_file, m_token_line, m_token_column };
        }
    case '{':
        advance();
        return Token { Token::Type::LCurlyBrace, m_file, m_token_line, m_token_column };
    case '[': {
        advance();
        switch (current_char()) {
        case ']':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::LBracketRBracketEqual, m_file, m_token_line, m_token_column };
            default:
                auto token = Token { Token::Type::LBracketRBracket, m_file, m_token_line, m_token_column };
                token.set_whitespace_precedes(m_whitespace_precedes);
                return token;
            }
        default:
            auto token = Token { Token::Type::LBracket, m_file, m_token_line, m_token_column };
            token.set_whitespace_precedes(m_whitespace_precedes);
            return token;
        }
    }
    case '(':
        advance();
        return Token { Token::Type::LParen, m_file, m_token_line, m_token_column };
    case '}':
        advance();
        return Token { Token::Type::RCurlyBrace, m_file, m_token_line, m_token_column };
    case ']':
        advance();
        return Token { Token::Type::RBracket, m_file, m_token_line, m_token_column };
    case ')':
        advance();
        return Token { Token::Type::RParen, m_file, m_token_line, m_token_column };
    case '\n': {
        advance();
        auto token = Token { Token::Type::Eol, m_file, m_token_line, m_token_column };
        while (m_index < m_index_after_heredoc)
            advance();
        return token;
    }
    case ';':
        advance();
        return Token { Token::Type::Semicolon, m_file, m_token_line, m_token_column };
    case ',':
        advance();
        return Token { Token::Type::Comma, m_file, m_token_line, m_token_column };
    case '"':
        advance();
        return consume_double_quoted_string('"');
    case '\'':
        advance();
        return consume_single_quoted_string('\'');
    case '`': {
        advance();
        auto token = consume_double_quoted_string('`');
        token.set_type(Token::Type::Shell);
        return token;
    }
    case '#':
        char c;
        do {
            advance();
            c = current_char();
        } while (c && c != '\n' && c != '\r');
        return Token { Token::Type::Comment, m_file, m_token_line, m_token_column };
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
            return Token { Token::Type::Invalid, current_char(), m_file, m_cursor_line, m_cursor_column };
        }
        return token;
    }
    };
    if (!token) {
        if (match(12, "__ENCODING__"))
            return Token { Token::Type::ENCODINGKeyword, m_file, m_token_line, m_token_column };
        else if (match(8, "__LINE__"))
            return Token { Token::Type::LINEKeyword, m_file, m_token_line, m_token_column };
        else if (match(8, "__FILE__"))
            return Token { Token::Type::FILEKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "BEGIN"))
            return Token { Token::Type::BEGINKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "END"))
            return Token { Token::Type::ENDKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "alias"))
            return Token { Token::Type::AliasKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "and"))
            return Token { Token::Type::AndKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "begin"))
            return Token { Token::Type::BeginKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "break"))
            return Token { Token::Type::BreakKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "case"))
            return Token { Token::Type::CaseKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "class"))
            return Token { Token::Type::ClassKeyword, m_file, m_token_line, m_token_column };
        else if (match(8, "defined?"))
            return Token { Token::Type::DefinedKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "def"))
            return Token { Token::Type::DefKeyword, m_file, m_token_line, m_token_column };
        else if (match(2, "do"))
            return Token { Token::Type::DoKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "else"))
            return Token { Token::Type::ElseKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "elsif"))
            return Token { Token::Type::ElsifKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "end"))
            return Token { Token::Type::EndKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "ensure"))
            return Token { Token::Type::EnsureKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "false"))
            return Token { Token::Type::FalseKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "for"))
            return Token { Token::Type::ForKeyword, m_file, m_token_line, m_token_column };
        else if (match(2, "if"))
            return Token { Token::Type::IfKeyword, m_file, m_token_line, m_token_column };
        else if (match(2, "in"))
            return Token { Token::Type::InKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "module"))
            return Token { Token::Type::ModuleKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "next"))
            return Token { Token::Type::NextKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "nil"))
            return Token { Token::Type::NilKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "not"))
            return Token { Token::Type::NotKeyword, m_file, m_token_line, m_token_column };
        else if (match(2, "or"))
            return Token { Token::Type::OrKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "redo"))
            return Token { Token::Type::RedoKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "rescue"))
            return Token { Token::Type::RescueKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "retry"))
            return Token { Token::Type::RetryKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "return"))
            return Token { Token::Type::ReturnKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "self"))
            return Token { Token::Type::SelfKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "super"))
            return Token { Token::Type::SuperKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "then"))
            return Token { Token::Type::ThenKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "true"))
            return Token { Token::Type::TrueKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "undef"))
            return Token { Token::Type::UndefKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "unless"))
            return Token { Token::Type::UnlessKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "until"))
            return Token { Token::Type::UntilKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "when"))
            return Token { Token::Type::WhenKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "while"))
            return Token { Token::Type::WhileKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "yield"))
            return Token { Token::Type::YieldKeyword, m_file, m_token_line, m_token_column };
        auto c = current_char();
        bool symbol_key = false;
        if ((c >= 'a' && c <= 'z') || c == '_') {
            return consume_bare_name();
        } else if (c >= 'A' && c <= 'Z') {
            return consume_constant();
        } else {
            auto buf = consume_non_whitespace();
            auto token = Token { Token::Type::Invalid, buf, m_file, m_token_line, m_token_column };
            return token;
        }
    }
    TM_UNREACHABLE();
}

Token Lexer::consume_symbol() {
    char c = current_char();
    SharedPtr<String> buf = new String("");
    auto gobble = [&buf, this](char c) -> char { buf->append_char(c); advance(); return current_char(); };
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
    case '~':
        c = gobble(c);
        if (c == '@') advance();
        break;
    case '+':
    case '-': {
        c = gobble(c);
        if (c == '@') gobble(c);
        break;
    }
    case '&':
    case '|':
    case '^':
    case '%':
    case '/': {
        gobble(c);
        break;
    }
    case '*':
        c = gobble(c);
        if (c == '*')
            gobble(c);
        break;
    case '=':
        switch (peek()) {
        case '=':
            c = gobble(c);
            c = gobble(c);
            if (c == '=') gobble(c);
            break;
        case '~':
            c = gobble(c);
            gobble(c);
            break;
        default:
            return Token { Token::Type::Invalid, c, m_file, m_token_line, m_token_column };
        }
        break;
    case '!':
        c = gobble(c);
        switch (c) {
        case '=':
        case '~':
        case '@':
            gobble(c);
        default:
            break;
        }
        break;
    case '>':
        c = gobble(c);
        switch (c) {
        case '=':
        case '>':
            gobble(c);
        default:
            break;
        }
        break;
    case '<':
        c = gobble(c);
        switch (c) {
        case '=':
            c = gobble(c);
            if (c == '>') gobble(c);
            break;
        case '<':
            gobble(c);
        default:
            break;
        }
        break;
    case '[':
        if (peek() == ']') {
            c = gobble(c);
            c = gobble(c);
            if (c == '=') gobble(c);
        } else {
            return Token { Token::Type::TernaryColon, m_file, m_token_line, m_token_column };
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
    return Token { Token::Type::Symbol, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_word(Token::Type type) {
    char c = current_char();
    SharedPtr<String> buf = new String("");
    do {
        buf->append_char(c);
        advance();
        c = current_char();
    } while (is_identifier_char(c));
    switch (c) {
    case '?':
    case '!':
        advance();
        buf->append_char(c);
        break;
    default:
        break;
    }
    return Token { type, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_bare_name() {
    auto token = consume_word(Token::Type::BareName);
    auto c = current_char();
    if (c == ':' && peek() != ':') {
        advance();
        token.set_type(Token::Type::SymbolKey);
    }
    return token;
}

Token Lexer::consume_constant() {
    auto token = consume_word(Token::Type::Constant);
    auto c = current_char();
    if (c == ':' && peek() != ':') {
        advance();
        token.set_type(Token::Type::SymbolKey);
    }
    return token;
}

Token Lexer::consume_global_variable() {
    switch (peek()) {
    case '?':
    case '!':
    case '=': {
        advance();
        SharedPtr<String> buf = new String("$");
        buf->append_char(current_char());
        advance();
        return Token { Token::Type::GlobalVariable, buf, m_file, m_token_line, m_token_column };
    }
    default: {
        return consume_word(Token::Type::GlobalVariable);
    }
    }
}

bool is_valid_heredoc(bool with_dash, SharedPtr<String> doc, String heredoc_name) {
    if (!doc->ends_with(heredoc_name))
        return false;
    if (doc->length() - heredoc_name.length() == 0)
        return true;
    auto prefix = (*doc)[doc->length() - heredoc_name.length() - 1];
    return with_dash ? isspace(prefix) : prefix == '\n';
}

size_t get_heredoc_indent(SharedPtr<String> doc) {
    if (doc->is_empty())
        return 0;
    size_t heredoc_indent = std::numeric_limits<size_t>::max();
    size_t line_begin = 0;
    size_t line_indent = 0;
    bool maybe_blank_line = true;
    for (size_t i = 0; i < doc->length(); i++) {
        char c = (*doc)[i];
        if (c == '\n') {
            if (!maybe_blank_line && line_indent < heredoc_indent)
                heredoc_indent = line_indent;
            line_begin = i + 1;
            line_indent = 0;
            maybe_blank_line = true;
        } else if (isspace(c)) {
            if (maybe_blank_line)
                line_indent++;
        } else {
            maybe_blank_line = false;
        }
    }
    return heredoc_indent;
}

void dedent_heredoc(SharedPtr<String> &doc) {
    size_t heredoc_indent = get_heredoc_indent(doc);
    if (heredoc_indent == 0)
        return;
    SharedPtr<String> new_doc = new String("");
    size_t line_begin = 0;
    for (size_t i = 0; i < doc->length(); i++) {
        char c = (*doc)[i];
        if (c == '\n') {
            line_begin += heredoc_indent;
            if (line_begin < i)
                new_doc->append(doc->substring(line_begin, i - line_begin));
            new_doc->append_char('\n');
            line_begin = i + 1;
        }
    }
    doc = new_doc;
}

Token Lexer::consume_heredoc() {
    bool with_dash = false;
    bool should_dedent = false;
    switch (current_char()) {
    case '-':
        advance();
        with_dash = true;
        break;
    case '~':
        advance();
        with_dash = true;
        should_dedent = true;
        break;
    }

    Token::Type type;
    char delimiter = 0;
    String heredoc_name = "";
    switch (current_char()) {
    case '"':
        type = Token::Type::DoubleQuotedString;
        delimiter = '"';
        break;
    case '`':
        type = Token::Type::Shell;
        delimiter = '`';
        break;
    case '\'':
        type = Token::Type::String;
        delimiter = '\'';
        break;
    default:
        type = Token::Type::DoubleQuotedString;
        heredoc_name = String(consume_word(Token::Type::BareName).literal());
    }

    if (delimiter) {
        advance();
        char c = current_char();
        while (c != delimiter) {
            switch (c) {
            case '\n':
            case '\r':
            case 0:
                return Token { Token::Type::UnterminatedString, "heredoc identifier", m_file, m_token_line, m_token_column };
            default:
                advance();
                heredoc_name.append_char(c);
                c = current_char();
            }
        }
        advance();
    }

    SharedPtr<String> doc = new String("");
    size_t heredoc_index = m_index;
    auto get_char = [&heredoc_index, this]() { return (heredoc_index >= m_size) ? 0 : m_input->at(heredoc_index); };

    // start consuming the heredoc on the next line
    while (get_char() != '\n') {
        if (heredoc_index >= m_size)
            return Token { Token::Type::UnterminatedString, "heredoc", m_file, m_token_line, m_token_column };
        heredoc_index++;
    }
    heredoc_index++;

    // consume the heredoc until we find the delimiter, either '\n' (if << was used) or any whitespace (if <<- was used) followed by "DELIM\n"
    for (;;) {
        if (heredoc_index >= m_size) {
            if (is_valid_heredoc(with_dash, doc, heredoc_name))
                break;
            return Token { Token::Type::UnterminatedString, doc, m_file, m_token_line, m_token_column };
        }
        char c = get_char();
        heredoc_index++;
        if (c == '\n' && is_valid_heredoc(with_dash, doc, heredoc_name))
            break;
        doc->append_char(c);
    }

    // chop the delimiter and any trailing space off the string
    doc->truncate(doc->length() - heredoc_name.length());
    doc->strip_trailing_spaces();

    if (should_dedent)
        dedent_heredoc(doc);

    // we have to keep tokenizing on the line where the heredoc was started, and then jump to the line after the heredoc
    // this index is used to do that
    m_index_after_heredoc = heredoc_index;

    auto token = Token { type, doc, m_file, m_token_line, m_token_column };
    return token;
}

Token Lexer::consume_numeric(bool negative) {
    size_t start_index = m_index;
    long long number = 0;
    if (current_char() == '0') {
        switch (peek()) {
        case 'd':
        case 'D': {
            advance(2);
            char c = current_char();
            if (!isdigit(c))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
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
            return Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
        }
        case 'o':
        case 'O': {
            advance(2);
            char c = current_char();
            if (!(c >= '0' && c <= '7'))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
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
            return Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
        }
        case 'x':
        case 'X': {
            advance(2);
            char c = current_char();
            if (!isxdigit(c))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
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
            return Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
        }
        case 'b':
        case 'B': {
            advance(2);
            char c = current_char();
            if (c != '0' && c != '1')
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
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
            return Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
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
        c = current_char();
        double dbl = number;
        int place = 10; // tenths
        do {
            dbl += (double)(c - 48) / place;
            place *= 10;
            advance();
            c = current_char();
            if (c == '_') {
                advance();
                c = current_char();
            }
        } while (isdigit(c));
        if (negative)
            dbl *= -1;
        return Token { Token::Type::Float, dbl, m_file, m_token_line, m_token_column };
    } else {
        if (negative)
            number *= -1;
        return Token { Token::Type::Integer, number, m_file, m_token_line, m_token_column };
    }
}

Token Lexer::consume_double_quoted_string(char delimiter) {
    SharedPtr<String> buf = new String("");
    char c = current_char();
    while (c) {
        if (c == '\\') {
            advance();
            c = current_char();
            switch (c) {
            case 'n':
                buf->append_char('\n');
                break;
            case 't':
                buf->append_char('\t');
                break;
            default:
                buf->append_char(c);
                break;
            }
        } else if (c == delimiter) {
            advance();
            auto token = Token { Token::Type::DoubleQuotedString, buf, m_file, m_token_line, m_token_column };
            return token;
        } else {
            buf->append_char(c);
        }
        advance();
        c = current_char();
    }
    return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_single_quoted_string(char delimiter) {
    SharedPtr<String> buf = new String("");
    char c = current_char();
    while (c) {
        if (c == '\\') {
            advance();
            c = current_char();
            switch (c) {
            case '\\':
            case '\'':
                buf->append_char(c);
                break;
            default:
                buf->append_char('\\');
                buf->append_char(c);
                break;
            }
        } else if (c == delimiter) {
            advance();
            return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column };
        } else {
            buf->append_char(c);
        }
        advance();
        c = current_char();
    }
    return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_quoted_array_without_interpolation(char delimiter, Token::Type type) {
    SharedPtr<String> buf = new String("");
    char c = current_char();
    bool seen_space = false;
    bool seen_start = false;
    for (;;) {
        if (!c) return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column };
        if (c == delimiter) {
            advance();
            break;
        }
        switch (c) {
        case ' ':
        case '\t':
        case '\n':
            if (!seen_space && seen_start) buf->append_char(' ');
            seen_space = true;
            break;
        default:
            buf->append_char(c);
            seen_space = false;
            seen_start = true;
        }
        advance();
        c = current_char();
    }
    if (!buf->is_empty() && buf->last_char() == ' ') buf->chomp();
    return Token { type, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_quoted_array_with_interpolation(char delimiter, Token::Type type) {
    SharedPtr<String> buf = new String("");
    char c = current_char();
    bool seen_space = false;
    bool seen_start = false;
    for (;;) {
        if (!c) return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column };
        if (c == delimiter) {
            advance();
            break;
        }
        switch (c) {
        case ' ':
        case '\t':
        case '\n':
            if (!seen_space && seen_start) buf->append_char(' ');
            seen_space = true;
            break;
        default:
            buf->append_char(c);
            seen_space = false;
            seen_start = true;
        }
        advance();
        c = current_char();
    }
    if (!buf->is_empty() && buf->last_char() == ' ') buf->chomp();
    return Token { type, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_regexp(char delimiter) {
    SharedPtr<String> buf = new String("");
    char c = current_char();
    while (c) {
        if (c == '\\') {
            advance();
            c = current_char();
            switch (c) {
            case '/':
                buf->append_char(c);
                break;
            default:
                buf->append_char('\\');
                buf->append_char(c);
                break;
            }
        } else if (c == delimiter) {
            advance();
            SharedPtr<String> options = new String("");
            while ((c = current_char())) {
                if (c == 'i' || c == 'm' || c == 'x' || c == 'o') {
                    options->append_char(c);
                    advance();
                } else {
                    break;
                }
            }
            auto token = Token { Token::Type::Regexp, buf, m_file, m_token_line, m_token_column };
            if (!options->is_empty())
                token.set_options(options);
            return token;
        } else {
            buf->append_char(c);
        }
        advance();
        c = current_char();
    }
    return Token { Token::Type::UnterminatedRegexp, buf, m_file, m_token_line, m_token_column };
}

SharedPtr<String> Lexer::consume_non_whitespace() {
    char c = current_char();
    SharedPtr<String> buf = new String("");
    do {
        buf->append_char(c);
        advance();
        c = current_char();
    } while (c && c != ' ' && c != '\t' && c != '\n' && c != '\r');
    return buf;
}

SharedPtr<Vector<Token>> InterpolatedStringLexer::tokens() {
    SharedPtr<Vector<Token>> tokens = new Vector<Token> {};
    SharedPtr<String> raw = new String("");
    while (m_index < m_size) {
        char c = current_char();
        if (c == '#' && peek() == '{') {
            if (!raw->is_empty() || tokens->is_empty()) {
                tokens->push(Token { Token::Type::String, new String(raw.ref()), m_file, m_line, m_column });
                *raw = "";
            }
            m_index += 2;
            tokenize_interpolation(tokens);
        } else {
            raw->append_char(c);
            m_index++;
        }
    }
    if (!raw->is_empty())
        tokens->push(Token { Token::Type::String, raw, m_file, m_line, m_column });
    return tokens;
}

void InterpolatedStringLexer::tokenize_interpolation(SharedPtr<Vector<Token>> tokens) {
    size_t start_index = m_index;
    size_t curly_brace_count = 1;
    while (m_index < m_size && curly_brace_count > 0) {
        char c = current_char();
        switch (c) {
        case '{':
            curly_brace_count++;
            break;
        case '}':
            curly_brace_count--;
            break;
        case '\\':
            m_index++;
            break;
        }
        m_index++;
    }
    if (curly_brace_count > 0) {
        fprintf(stderr, "missing } in string interpolation in %s#%zu\n", m_file->c_str(), m_line + 1);
        abort();
    }
    // m_input = "#{:foo} bar"
    //                   ^ m_index = 7
    //              ^ start_index = 2
    // part = ":foo" (len = 4)
    size_t len = m_index - start_index - 1;
    auto part = m_input->substring(start_index, len);
    auto lexer = new Lexer { new String(part), m_file };
    tokens->push(Token { Token::Type::EvaluateToStringBegin, m_file, m_line, m_column });
    auto part_tokens = lexer->tokens();
    for (auto token : *part_tokens) {
        if (token.is_eof()) {
            tokens->push(Token { Token::Type::Eol, m_file, m_line, m_column });
            break;
        } else {
            tokens->push(token);
        }
    }
    tokens->push(Token { Token::Type::EvaluateToStringEnd, m_file, m_line, m_column });
}

}
