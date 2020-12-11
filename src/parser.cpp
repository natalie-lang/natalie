#include "natalie/parser.hpp"

namespace Natalie {

Parser::Node *Parser::parse_expression(Env *env, Parser::Precedence precedence, Vector<SymbolValue *> *locals) {
#ifdef NAT_DEBUG_PARSER
    printf("entering parse_expression with precedence = %d, current token = %s\n", precedence, current_token().to_ruby(env)->inspect_str(env));
#endif
    skip_newlines();

    auto null_fn = null_denotation(current_token().type());
    if (!null_fn) {
        raise_unexpected(env, "null_fn");
    }

    Node *left = (this->*null_fn)(env, locals);

    while (current_token().is_valid() && precedence < get_precedence()) {
#ifdef NAT_DEBUG_PARSER
        printf("while loop: current token = %s\n", current_token().to_ruby(env)->inspect_str(env));
#endif
        auto left_fn = left_denotation(current_token().type());
        if (!left_fn) {
            raise_unexpected(env, "left_fn");
        }

        left = (this->*left_fn)(env, left, locals);
    }
    return left;
}

Parser::Node *Parser::tree(Env *env) {
    auto tree = new Parser::BlockNode {};
    current_token().validate(env);
    auto locals = new Vector<SymbolValue *> {};
    while (!current_token().is_eof()) {
        auto exp = parse_expression(env, LOWEST, locals);
        tree->add_node(exp);
        current_token().validate(env);
        next_expression(env);
    }
    return tree;
}

Parser::Node *Parser::parse_def(Env *env, Vector<SymbolValue *> *) {
    advance();
    auto locals = new Vector<SymbolValue *> {};
    auto name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
    auto args = new Vector<Node *> {};
    if (current_token().is_lparen()) {
        advance();
        if (!current_token().is_identifier())
            raise_unexpected(env, "parse_def first arg identifier");
        args->push(parse_identifier(env, locals));
        while (current_token().is_comma()) {
            advance();
            if (!current_token().is_identifier())
                raise_unexpected(env, "parse_def 2nd+ arg identifier");
            args->push(parse_identifier(env, locals));
        };
        if (!current_token().is_rparen())
            raise_unexpected(env, "parse_def rparen");
        advance();
    } else if (current_token().is_identifier()) {
        args->push(parse_identifier(env, locals));
        while (current_token().is_comma()) {
            advance();
            if (!current_token().is_identifier())
                raise_unexpected(env, "parse_def 2nd+ arg identifier");
            args->push(parse_identifier(env, locals));
        };
    }
    auto body = static_cast<BlockNode *>(parse_body(env, locals));
    return new DefNode { name, args, body };
};

Parser::Node *Parser::parse_body(Env *env, Vector<SymbolValue *> *locals) {
    auto body = new BlockNode {};
    current_token().validate(env);
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_end_keyword()) {
        auto exp = parse_expression(env, LOWEST, locals);
        body->add_node(exp);
        current_token().validate(env);
        next_expression(env);
    }
    if (!current_token().is_end_keyword())
        raise_unexpected(env, "parse_body end");
    advance();
    return body;
}

Parser::Node *Parser::parse_lit(Env *env, Vector<SymbolValue *> *locals) {
    Value *value;
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::Integer:
        value = new IntegerValue { env, token.get_integer() };
        break;
    case Token::Type::Float:
        value = new FloatValue { env, token.get_double() };
        break;
    default:
        NAT_UNREACHABLE();
    }
    advance();
    return new LiteralNode { value };
};

Parser::Node *Parser::parse_string(Env *env, Vector<SymbolValue *> *locals) {
    auto lit = new StringNode { new StringValue { env, current_token().literal() } };
    advance();
    return lit;
};

Parser::Node *Parser::parse_identifier(Env *env, Vector<SymbolValue *> *locals) {
    bool is_lvar = false;
    auto name_symbol = SymbolValue::intern(env, current_token().literal());
    for (auto local : *locals) {
        if (local == name_symbol) {
            is_lvar = true;
            break;
        }
    }
    auto identifier = new IdentifierNode { current_token(), is_lvar };
    advance();
    return identifier;
};

Parser::Node *Parser::parse_infix_expression(Env *env, Node *left, Vector<SymbolValue *> *locals) {
    auto op = current_token();
    auto precedence = get_precedence();
    advance();
    auto right = parse_expression(env, precedence, locals);
    auto node = new CallNode {
        left,
        op.type_value(env),
    };
    node->add_arg(right);
    return node;
};

Parser::Node *Parser::parse_assignment_expression(Env *env, Node *left, Vector<SymbolValue *> *locals) {
    auto left_identifier = static_cast<IdentifierNode *>(left);
    if (left_identifier->token_type() == Token::Type::Identifier)
        locals->push(SymbolValue::intern(env, left_identifier->name()));
    advance();
    auto right = parse_expression(env, ASSIGNMENT, locals);
    return new AssignmentNode {
        left_identifier,
        right,
    };
};

Parser::Node *Parser::parse_grouped_expression(Env *env, Vector<SymbolValue *> *locals) {
    advance();
    auto exp = parse_expression(env, LOWEST, locals);
    if (!current_token().is_valid()) {
        fprintf(stderr, "expected ), but got EOF\n");
        abort();
    } else if (current_token().type() != Token::Type::RParen) {
        fprintf(stderr, "expected ), but got %s\n", current_token().type_value(env)->c_str());
        abort();
    }
    advance();
    return exp;
};

Parser::parse_null_fn Parser::null_denotation(Token::Type type) {
    using Type = Token::Type;
    switch (type) {
    case Type::DefKeyword:
        return &Parser::parse_def;
    case Type::LParen:
        return &Parser::parse_grouped_expression;
    case Type::ClassVariable:
    case Type::Constant:
    case Type::GlobalVariable:
    case Type::Identifier:
    case Type::InstanceVariable:
        return &Parser::parse_identifier;
    case Type::Integer:
    case Type::Float:
        return &Parser::parse_lit;
    case Type::String:
        return &Parser::parse_string;
    default:
        return nullptr;
    }
};

Parser::parse_left_fn Parser::left_denotation(Token::Type type) {
    using Type = Token::Type;
    switch (type) {
    case Type::Plus:
    case Type::Minus:
    case Type::Multiply:
    case Type::Divide:
        return &Parser::parse_infix_expression;
    case Type::Equal:
        return &Parser::parse_assignment_expression;
    default:
        return nullptr;
    }
};

Token Parser::current_token() {
    if (m_index < m_tokens->size()) {
        return (*m_tokens)[m_index];
    } else {
        return Token::invalid();
    }
}

Token Parser::peek_token() {
    if (m_index + 1 < m_tokens->size()) {
        return (*m_tokens)[m_index + 1];
    } else {
        return Token::invalid();
    }
}

void Parser::next_expression(Env *env) {
    auto token = current_token();
    if (!token.is_eol() && !token.is_eof())
        raise_unexpected(env, "end-of-line");
    skip_newlines();
}

void Parser::skip_newlines() {
    while (current_token().is_eol())
        advance();
}

void Parser::raise_unexpected(Env *env, const char *expected) {
    auto line = current_token().line() + 1;
    auto type = current_token().type_value(env);
#ifdef NAT_DEBUG_PARSER
    printf("DEBUG: %s expected\n", expected);
#endif
    if (type == SymbolValue::intern(env, "EOF")) {
        env->raise("SyntaxError", "%d: syntax error, unexpected end-of-input", line);
    } else {
        env->raise("SyntaxError", "%d: syntax error, unexpected '%s'", line, type->c_str());
    }
}

}
