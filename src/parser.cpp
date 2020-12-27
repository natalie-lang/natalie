#include "natalie/parser.hpp"

namespace Natalie {

Parser::Node *Parser::parse_expression(Env *env, Parser::Precedence precedence, LocalsVectorPtr locals) {
#ifdef NAT_DEBUG_PARSER
    printf("entering parse_expression with precedence = %d, current token = %s\n", precedence, current_token().to_ruby(env)->inspect_str(env));
#endif
    skip_newlines();

    auto null_fn = null_denotation(current_token().type(), precedence);
    if (!null_fn) {
        raise_unexpected(env, "expression");
    }

    Node *left = (this->*null_fn)(env, locals);

    if (left->type() == Node::Type::Identifier && !current_token().is_end_of_expression() && get_precedence() == LOWEST && precedence == LOWEST) {
        left = parse_call_expression_without_parens(env, left, locals);
    }

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
    skip_newlines();
    while (!current_token().is_eof()) {
        auto exp = parse_expression(env, LOWEST, locals);
        tree->add_node(exp);
        current_token().validate(env);
        next_expression(env);
    }
    return tree;
}

Parser::BlockNode *Parser::parse_body(Env *env, LocalsVectorPtr locals, Token::Type end_token_type) {
    auto body = new BlockNode {};
    current_token().validate(env);
    skip_newlines();
    while (!current_token().is_eof() && current_token().type() != end_token_type) {
        auto exp = parse_expression(env, LOWEST, locals);
        body->add_node(exp);
        current_token().validate(env);
        if (end_token_type == Token::Type::EndKeyword) {
            next_expression(env);
        } else {
            auto token = current_token();
            if (token.type() != end_token_type && !token.is_end_of_expression())
                raise_unexpected(env, "end-of-line");
            skip_newlines();
        }
    }
    if (current_token().type() != end_token_type)
        raise_unexpected(env, "body end");
    advance();
    return body;
}

Parser::Node *Parser::parse_array(Env *env, LocalsVectorPtr locals) {
    advance();
    auto array = new ArrayNode {};
    if (current_token().type() != Token::Type::RBracket) {
        array->add_node(parse_expression(env, ARRAY, locals));
        while (current_token().type() == Token::Type::Comma) {
            advance();
            array->add_node(parse_expression(env, ARRAY, locals));
        }
    }
    expect(env, Token::Type::RBracket, "array closing bracket");
    advance();
    return array;
}

Parser::Node *Parser::parse_block_pass(Env *env, LocalsVectorPtr locals) {
    advance();
    switch (current_token().type()) {
    case Token::Type::ClassVariable:
    case Token::Type::Constant:
    case Token::Type::GlobalVariable:
    case Token::Type::Identifier:
    case Token::Type::InstanceVariable:
        return new BlockPassNode { parse_expression(env, LOWEST, locals) };
    case Token::Type::Symbol:
        return new BlockPassNode { parse_symbol(env, locals) };
    default:
        expect(env, Token::Type::Identifier, "block");
    }
    NAT_UNREACHABLE();
}

Parser::Node *Parser::parse_bool(Env *env, LocalsVectorPtr) {
    switch (current_token().type()) {
    case Token::Type::TrueKeyword:
        advance();
        return new TrueNode {};
    case Token::Type::FalseKeyword:
        advance();
        return new FalseNode {};
    default:
        NAT_UNREACHABLE();
    }
}

Parser::Node *Parser::parse_class(Env *env, LocalsVectorPtr) {
    advance();
    auto locals = new Vector<SymbolValue *> {};
    if (current_token().type() != Token::Type::Constant)
        env->raise("SyntaxError", "class/module name must be CONSTANT");
    auto name = static_cast<ConstantNode *>(parse_constant(env, locals));
    Node *superclass;
    if (current_token().type() == Token::Type::LessThan) {
        advance();
        superclass = parse_expression(env, LOWEST, locals);
    } else {
        superclass = new NilNode {};
    }
    auto body = parse_body(env, locals);
    return new ClassNode { name, superclass, body };
};

Parser::Node *Parser::parse_comma_separated_identifiers(Env *env, LocalsVectorPtr locals) {
    auto list = new CommaSeparatedIdentifiersNode {};
    list->add_node(parse_identifier(env, locals));
    while (current_token().is_comma()) {
        advance();
        switch (current_token().type()) {
        case Token::Type::ClassVariable:
        case Token::Type::Constant:
        case Token::Type::GlobalVariable:
        case Token::Type::Identifier:
        case Token::Type::InstanceVariable:
            list->add_node(parse_identifier(env, locals));
            break;
        default:
            expect(env, Token::Type::Identifier, "assignment identifier");
        }
    }
    return list;
}

Parser::Node *Parser::parse_constant(Env *env, LocalsVectorPtr locals) {
    auto node = new ConstantNode { current_token() };
    advance();
    return node;
};

Parser::Node *Parser::parse_def(Env *env, LocalsVectorPtr) {
    advance();
    auto locals = new Vector<SymbolValue *> {};
    auto name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
    Vector<Node *> *args;
    if (current_token().is_lparen()) {
        advance();
        expect(env, Token::Type::Identifier, "parse_def first arg identifier");
        args = parse_def_args(env, locals);
        expect(env, Token::Type::RParen, "parse_def rparen");
        advance();
    } else if (current_token().is_identifier()) {
        args = parse_def_args(env, locals);
    } else {
        args = new Vector<Node *> {};
    }
    auto body = parse_body(env, locals);
    return new DefNode { name, args, body };
};

Vector<Parser::Node *> *Parser::parse_def_args(Env *env, LocalsVectorPtr locals) {
    auto args = new Vector<Node *> {};
    auto arg = static_cast<IdentifierNode *>(parse_identifier(env, locals));
    args->push(arg);
    locals->push(SymbolValue::intern(env, arg->name()));
    while (current_token().is_comma()) {
        advance();
        expect(env, Token::Type::Identifier, "parse_def 2nd+ arg identifier");
        auto arg = static_cast<IdentifierNode *>(parse_identifier(env, locals));
        args->push(arg);
        locals->push(SymbolValue::intern(env, arg->name()));
    };
    return args;
}

Parser::Node *Parser::parse_group(Env *env, LocalsVectorPtr locals) {
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

Parser::Node *Parser::parse_hash(Env *env, LocalsVectorPtr locals) {
    advance();
    auto hash = new HashNode {};
    if (current_token().type() != Token::Type::RCurlyBrace) {
        if (current_token().type() == Token::Type::SymbolKey) {
            hash->add_node(parse_symbol(env, locals));
        } else {
            hash->add_node(parse_expression(env, HASH, locals));
            expect(env, Token::Type::HashRocket, "hash rocket");
            advance();
        }
        hash->add_node(parse_expression(env, HASH, locals));
        while (current_token().type() == Token::Type::Comma) {
            advance();
            if (current_token().type() == Token::Type::SymbolKey) {
                hash->add_node(parse_symbol(env, locals));
            } else {
                hash->add_node(parse_expression(env, HASH, locals));
                expect(env, Token::Type::HashRocket, "hash rocket");
                advance();
            }
            hash->add_node(parse_expression(env, HASH, locals));
        }
    }
    expect(env, Token::Type::RCurlyBrace, "hash closing curly brace");
    advance();
    return hash;
}

Parser::Node *Parser::parse_identifier(Env *env, LocalsVectorPtr locals) {
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

Parser::Node *Parser::parse_if(Env *env, LocalsVectorPtr locals) {
    advance();
    auto condition = parse_expression(env, LOWEST, locals);
    next_expression(env);
    auto true_expr = parse_if_body(env, locals);
    Node *false_expr;
    if (current_token().is_elsif_keyword()) {
        false_expr = parse_if(env, locals);
        return new IfNode { condition, true_expr, false_expr };
    } else {
        if (current_token().is_else_keyword()) {
            advance();
            false_expr = parse_if_body(env, locals);
        } else {
            false_expr = new NilNode {};
        }
        expect(env, Token::Type::EndKeyword, "if end");
        advance();
        return new IfNode { condition, true_expr, false_expr };
    }
}

Parser::Node *Parser::parse_if_body(Env *env, LocalsVectorPtr locals) {
    auto body = new BlockNode {};
    current_token().validate(env);
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_elsif_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword()) {
        auto exp = parse_expression(env, LOWEST, locals);
        body->add_node(exp);
        current_token().validate(env);
        next_expression(env);
    }
    if (!current_token().is_elsif_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword())
        raise_unexpected(env, "if end");
    return body->has_one_node() ? (*body->nodes())[0] : body;
}

Parser::Node *Parser::parse_lit(Env *env, LocalsVectorPtr locals) {
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

Parser::Node *Parser::parse_module(Env *env, LocalsVectorPtr) {
    advance();
    auto locals = new Vector<SymbolValue *> {};
    if (current_token().type() != Token::Type::Constant)
        env->raise("SyntaxError", "class/module name must be CONSTANT");
    auto name = static_cast<ConstantNode *>(parse_constant(env, locals));
    auto body = parse_body(env, locals);
    return new ModuleNode { name, body };
};

Parser::Node *Parser::parse_return(Env *env, LocalsVectorPtr locals) {
    advance();
    if (current_token().is_end_of_expression())
        return new ReturnNode {};
    return new ReturnNode { parse_expression(env, LOWEST, locals) };
};

Parser::Node *Parser::parse_string(Env *env, LocalsVectorPtr locals) {
    auto string = new StringNode { new StringValue { env, current_token().literal() } };
    advance();
    return string;
};

Parser::Node *Parser::parse_symbol(Env *env, LocalsVectorPtr locals) {
    auto symbol = new SymbolNode { SymbolValue::intern(env, current_token().literal()) };
    advance();
    return symbol;
};

Parser::Node *Parser::parse_word_array(Env *env, LocalsVectorPtr locals) {
    auto array = new ArrayNode {};
    auto literal = current_token().literal();
    size_t len = strlen(literal);
    if (len > 0) {
        StringValue *string = new StringValue { env };
        for (size_t i = 0; i < len; i++) {
            auto c = literal[i];
            switch (c) {
            case ' ':
                array->add_node(new StringNode { string });
                string = new StringValue { env };
                break;
            default:
                string->append_char(env, c);
            }
        }
        array->add_node(new StringNode { string });
    }
    advance();
    return array;
}

Parser::Node *Parser::parse_word_symbol_array(Env *env, LocalsVectorPtr locals) {
    auto array = new ArrayNode {};
    auto literal = current_token().literal();
    size_t len = strlen(literal);
    if (len > 0) {
        StringValue *string = new StringValue { env };
        for (size_t i = 0; i < len; i++) {
            auto c = literal[i];
            switch (c) {
            case ' ':
                array->add_node(new LiteralNode { SymbolValue::intern(env, string->c_str()) });
                string = new StringValue { env };
                break;
            default:
                string->append_char(env, c);
            }
        }
        array->add_node(new LiteralNode { SymbolValue::intern(env, string->c_str()) });
    }
    advance();
    return array;
}

Parser::Node *Parser::parse_assignment_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    switch (left->type()) {
    case Node::Type::Identifier: {
        auto left_identifier = static_cast<IdentifierNode *>(left);
        if (left_identifier->token_type() == Token::Type::Identifier)
            locals->push(SymbolValue::intern(env, left_identifier->name()));
        advance();
        return new AssignmentNode {
            left,
            parse_expression(env, ASSIGNMENT, locals),
        };
    }
    case Node::Type::CommaSeparatedIdentifiers: {
        advance();
        return new AssignmentNode {
            left,
            parse_expression(env, ASSIGNMENT, locals),
        };
    }
    default:
        NAT_UNREACHABLE();
    }
};

Parser::Node *Parser::parse_iter_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    bool curly_brace = current_token().type() == Token::Type::LCurlyBrace;
    advance();
    switch (left->type()) {
    case Node::Type::Identifier:
    case Node::Type::Call:
        break;
    default:
        raise_unexpected(env, "call");
    }
    auto end_token_type = curly_brace ? Token::Type::RCurlyBrace : Token::Type::EndKeyword;
    auto body = parse_body(env, locals, end_token_type);
    return new IterNode { left, body };
}

Parser::Node *Parser::parse_call_expression_with_parens(Env *env, Node *left, LocalsVectorPtr locals) {
    CallNode *call_node;
    switch (left->type()) {
    case Node::Type::Identifier:
        call_node = new CallNode {
            new NilNode {},
            SymbolValue::intern(env, static_cast<IdentifierNode *>(left)->name()),
        };
        break;
    case Node::Type::Call:
        call_node = static_cast<CallNode *>(left);
        break;
    default:
        NAT_UNREACHABLE();
    }
    advance();
    if (!current_token().is_rparen()) {
        auto arg = parse_expression(env, LOWEST, locals);
        call_node->add_arg(arg);
        while (current_token().is_comma()) {
            advance();
            auto arg = parse_expression(env, LOWEST, locals);
            call_node->add_arg(arg);
        }
    }
    expect(env, Token::Type::RParen, "call rparen");
    advance();
    return call_node;
}

Parser::Node *Parser::parse_call_expression_without_parens(Env *env, Node *left, LocalsVectorPtr locals) {
    CallNode *call_node;
    switch (left->type()) {
    case Node::Type::Identifier:
        call_node = new CallNode {
            new NilNode {},
            SymbolValue::intern(env, static_cast<IdentifierNode *>(left)->name()),
        };
        break;
    case Node::Type::Call:
        call_node = static_cast<CallNode *>(left);
        break;
    default:
        NAT_UNREACHABLE();
    }
    switch (current_token().type()) {
    case Token::Type::Comma:
    case Token::Type::RCurlyBrace:
    case Token::Type::RBracket:
    case Token::Type::RParen:
    case Token::Type::Eof:
    case Token::Type::Eol:
        break;
    default:
        auto arg = parse_expression(env, LOWEST, locals);
        call_node->add_arg(arg);
        while (current_token().is_comma()) {
            advance();
            auto arg = parse_expression(env, LOWEST, locals);
            call_node->add_arg(arg);
        }
    }
    return call_node;
}

Parser::Node *Parser::parse_infix_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto op = current_token();
    auto precedence = get_precedence();
    advance();
    Node *right = nullptr;
    if (op.type() == Token::Type::Integer) {
        bool is_negative = op.get_integer() < 0;
        right = new LiteralNode { new IntegerValue { env, op.get_integer() * (is_negative ? -1 : 1) } };
        op = Token { is_negative ? Token::Type::Minus : Token::Type::Plus, op.line(), op.column() };
    } else if (op.type() == Token::Type::Float) {
        bool is_negative = op.get_double() < 0;
        right = new LiteralNode { new FloatValue { env, op.get_double() * (is_negative ? -1 : 1) } };
        op = Token { is_negative ? Token::Type::Minus : Token::Type::Plus, op.line(), op.column() };
    } else {
        right = parse_expression(env, precedence, locals);
    }
    auto node = new CallNode {
        left,
        op.type_value(env),
    };
    node->add_arg(right);
    return node;
};

Parser::Node *Parser::parse_range_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    return new RangeNode { left, parse_expression(env, LOWEST, locals), token.type() == Token::Type::DotDotDot };
}

Parser::Node *Parser::parse_send_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    advance();
    expect(env, Token::Type::Identifier, "send method name");
    auto name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
    auto call_node = new CallNode {
        left,
        SymbolValue::intern(env, name->name()),
    };

    if (!current_token().is_end_of_expression() && get_precedence() == LOWEST) {
        call_node = static_cast<CallNode *>(parse_call_expression_without_parens(env, call_node, locals));
    }

    return call_node;
}

Parser::Node *Parser::parse_ternary_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    expect(env, Token::Type::TernaryQuestion, "ternary question");
    advance();
    auto true_expr = parse_expression(env, TERNARY, locals);
    expect(env, Token::Type::TernaryColon, "ternary colon");
    advance();
    auto false_expr = parse_expression(env, TERNARY, locals);
    return new IfNode { left, true_expr, false_expr };
}

Parser::parse_null_fn Parser::null_denotation(Token::Type type, Precedence precedence) {
    using Type = Token::Type;
    switch (type) {
    case Type::LBracket:
        return &Parser::parse_array;
    case Type::BitwiseAnd:
        return &Parser::parse_block_pass;
    case Type::TrueKeyword:
    case Type::FalseKeyword:
        return &Parser::parse_bool;
    case Type::ClassKeyword:
        return &Parser::parse_class;
    case Type::DefKeyword:
        return &Parser::parse_def;
    case Type::LParen:
        return &Parser::parse_group;
    case Type::LCurlyBrace:
        return &Parser::parse_hash;
    case Type::ClassVariable:
    case Type::Constant:
    case Type::GlobalVariable:
    case Type::Identifier:
    case Type::InstanceVariable:
        if (peek_token().is_comma() && precedence == LOWEST) {
            return &Parser::parse_comma_separated_identifiers;
        } else {
            return &Parser::parse_identifier;
        }
    case Type::IfKeyword:
        return &Parser::parse_if;
    case Type::Integer:
    case Type::Float:
        return &Parser::parse_lit;
    case Type::ModuleKeyword:
        return &Parser::parse_module;
    case Type::ReturnKeyword:
        return &Parser::parse_return;
    case Type::String:
        return &Parser::parse_string;
    case Type::Symbol:
        return &Parser::parse_symbol;
    case Type::PercentLowerI:
    case Type::PercentUpperI:
        return &Parser::parse_word_symbol_array;
    case Type::PercentLowerW:
    case Type::PercentUpperW:
        return &Parser::parse_word_array;
    default:
        return nullptr;
    }
};

Parser::parse_left_fn Parser::left_denotation(Token::Type type) {
    using Type = Token::Type;
    switch (type) {
    case Type::Equal:
        return &Parser::parse_assignment_expression;
    case Type::LParen:
        return &Parser::parse_call_expression_with_parens;
    case Type::Plus:
    case Type::Minus:
    case Type::Multiply:
    case Type::Divide:
    case Type::EqualEqual:
    case Type::Exponent:
    case Type::LessThan:
    case Type::LessThanOrEqual:
    case Type::GreaterThan:
    case Type::GreaterThanOrEqual:
        return &Parser::parse_infix_expression;
    case Type::Integer:
        if (current_token().has_sign())
            return &Parser::parse_infix_expression;
        else
            return nullptr;
    case Type::Float:
        if (current_token().has_sign())
            return &Parser::parse_infix_expression;
        else
            return nullptr;
    case Type::DoKeyword:
    case Type::LCurlyBrace:
        return &Parser::parse_iter_expression;
    case Type::DotDot:
    case Type::DotDotDot:
        return &Parser::parse_range_expression;
    case Type::Dot:
        return &Parser::parse_send_expression;
    case Type::TernaryQuestion:
        return &Parser::parse_ternary_expression;
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
    if (!token.is_end_of_expression())
        raise_unexpected(env, "end-of-line");
    skip_newlines();
}

void Parser::skip_newlines() {
    while (current_token().is_eol())
        advance();
}

void Parser::expect(Env *env, Token::Type type, const char *expected) {
    if (current_token().type() != type)
        raise_unexpected(env, expected);
}

void Parser::raise_unexpected(Env *env, const char *expected) {
    auto line = current_token().line() + 1;
    auto type = current_token().type_value(env);
    if (type == SymbolValue::intern(env, "EOF")) {
        env->raise("SyntaxError", "%d: syntax error, unexpected end-of-input", line);
    } else {
        env->raise("SyntaxError", "%d: syntax error, unexpected '%s' (expected: '%s')", line, type->c_str(), expected);
    }
}
}
