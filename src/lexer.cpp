#include "natalie/lexer.hpp"
#include "natalie.hpp"
#include "natalie/token.hpp"

namespace Natalie {

Vector<Token *> *Lexer::tokens() {
    auto tokens = new Vector<Token *> {};
    bool skip_next_newline = false;
    for (;;) {
        auto token = next_token();
        if (token->is_comment())
            continue;

        // get rid of newlines after certain tokens
        if (skip_next_newline && token->is_newline())
            continue;
        if (skip_next_newline && !token->is_newline())
            skip_next_newline = false;

        // get rid of newlines before certain tokens
        while (token->can_follow_collapsible_newline() && !tokens->is_empty() && tokens->last()->is_newline())
            tokens->pop();

        // convert semicolons to eol tokens
        if (token->is_semicolon())
            token = new Token { Token::Type::Eol, token->file(), token->line(), token->column() };

        // break apart interpolations in double-quoted string
        if (token->can_have_interpolation()) {
            Token::Type begin_token_type = Token::Type::InterpolatedStringBegin;
            Token::Type end_token_type = Token::Type::InterpolatedStringEnd;
            if (token->type() == Token::Type::Shell) {
                begin_token_type = Token::Type::InterpolatedShellBegin;
                end_token_type = Token::Type::InterpolatedShellEnd;
            } else if (token->type() == Token::Type::Regexp) {
                begin_token_type = Token::Type::InterpolatedRegexpBegin;
                end_token_type = Token::Type::InterpolatedRegexpEnd;
            }
            auto string_lexer = InterpolatedStringLexer { token };
            tokens->push(new Token { begin_token_type, token->file(), token->line(), token->column() });
            for (auto token : *string_lexer.tokens()) {
                tokens->push(token);
            }
            tokens->push(new Token { end_token_type, token->file(), token->line(), token->column() });
            continue;
        }

        tokens->push(token);

        if (token->is_eof())
            return tokens;
        if (!token->is_valid())
            return tokens;
        if (token->can_precede_collapsible_newline())
            skip_next_newline = true;
    };
    NAT_UNREACHABLE();
}

void InterpolatedStringLexer::tokenize_interpolation(Vector<Token *> *tokens) {
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
    size_t len = m_index - start_index;
    char part[len];
    strncpy(part, m_input->c_str() + start_index, len);
    part[len - 1] = 0;
    auto lexer = new Lexer { part, m_file };
    tokens->push(new Token { Token::Type::EvaluateToStringBegin, m_file, m_line, m_column });
    for (auto token : *lexer->tokens()) {
        if (token->is_eof()) {
            tokens->push(new Token { Token::Type::Eol, m_file, m_line, m_column });
            break;
        } else {
            tokens->push(token);
        }
    }
    tokens->push(new Token { Token::Type::EvaluateToStringEnd, m_file, m_line, m_column });
}

}
