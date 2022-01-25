#include "natalie_parser/token.hpp"
#include "natalie_parser/parser.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

void Token::validate() {
    switch (m_type) {
    case Type::Invalid:
        throw Parser::SyntaxError { String::format("{}: syntax error, unexpected '{}'", m_line + 1, literal_or_blank()) };
    case Type::UnterminatedRegexp:
        throw Parser::SyntaxError { String::format("unterminated regexp meets end of file at line {} and column {}: {}", m_line, m_column, literal_or_blank()) };
    case Type::UnterminatedString:
        throw Parser::SyntaxError { String::format("unterminated string meets end of file at line {} and column {}: {}", m_line, m_column, literal_or_blank()) };
    default:
        assert(type_value()); // all other types should return a string for type_value()
        return;
    }
}

}
