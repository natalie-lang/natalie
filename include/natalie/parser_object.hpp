#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include "natalie_parser/lexer.hpp"
#include "natalie_parser/node.hpp"
#include "natalie_parser/parser.hpp"

namespace Natalie {

class ParserObject : public Object {
public:
    Value parse(Env *, Value, Value = nullptr);
    Value tokens(Env *, Value, Value);

private:
    Value node_to_ruby(Env *env, const NatalieParser::Node &node);
    Value token_to_ruby(Env *env, NatalieParser::Token &token, bool with_line_and_column_numbers = false);
    void validate_token(Env *env, NatalieParser::Token &token);
};

}
