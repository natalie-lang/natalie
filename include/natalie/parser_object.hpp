#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/node.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class ParserObject : public Object {
public:
    Value parse(Env *, Value, Value = nullptr);
    Value tokens(Env *, Value, Value);

private:
    Value token_to_ruby(Env *env, Token &token, bool with_line_and_column_numbers = false);
    void validate_token(Env *env, Token &token);

    Value node_to_ruby(Env *env, Node *node);
    SexpObject *build_args_sexp(Env *env, Node *parent, Vector<Node *> &args);
    SexpObject *build_assignment_sexp(Env *env, Node *parent, IdentifierNode *identifier);
    SexpObject *multiple_assignment_to_ruby_with_array(Env *env, MultipleAssignmentNode *node);
};

}
