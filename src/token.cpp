#include "natalie/token.hpp"
#include "natalie/managed_string.hpp"
#include "natalie/parser.hpp"

namespace Natalie {

void Token::validate() {
    switch (m_type) {
    case Type::Invalid:
        throw Parser::SyntaxError { ManagedString::format("{}: syntax error, unexpected '{}'", m_line + 1, literal_or_blank()) };
    case Type::UnterminatedRegexp:
        throw Parser::SyntaxError { ManagedString::format("unterminated regexp meets end of file at line {} and column {}: {}", m_line, m_column, literal_or_blank()) };
    case Type::UnterminatedString:
        throw Parser::SyntaxError { ManagedString::format("unterminated string meets end of file at line {} and column {}: {}", m_line, m_column, literal_or_blank()) };
    default:
        assert(type_value()); // all other types should return a string for type_value()
        return;
    }
}

void Token::validate_or_raise(Env *env) {
    try {
        validate();
    } catch (Parser::SyntaxError &e) {
        env->raise("SyntaxError", e.message());
    }
}

}
