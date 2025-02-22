#include "natalie.hpp"

namespace Natalie {

NumberParser::NumberParser(const StringObject *str)
    : m_str { str->string() } { }

FloatObject *NumberParser::string_to_f(const StringObject *str) {
    auto parser = NumberParser { str };
    auto result = strtod(parser.m_str.c_str(), nullptr);
    return new FloatObject { result };
}

}
