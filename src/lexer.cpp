#include "natalie/lexer.hpp"
#include "natalie.hpp"
#include "natalie/token.hpp"

namespace Natalie {

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
    NAT_UNREACHABLE();
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
