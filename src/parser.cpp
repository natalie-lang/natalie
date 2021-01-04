#include "natalie/parser.hpp"

namespace Natalie {

Node *Parser::parse_expression(Env *env, Parser::Precedence precedence, LocalsVectorPtr locals) {
#ifdef NAT_DEBUG_PARSER
    printf("entering parse_expression with precedence = %d, current token = %s\n", precedence, current_token().to_ruby(env)->inspect_str(env));
#endif
    skip_newlines();

    auto null_fn = null_denotation(current_token().type(), precedence);
    if (!null_fn) {
        raise_unexpected(env, "expression");
    }

    Node *left = (this->*null_fn)(env, locals);

    while (current_token().is_valid() && get_precedence(left) > precedence) {
#ifdef NAT_DEBUG_PARSER
        printf("while loop: current token = %s\n", current_token().to_ruby(env)->inspect_str(env));
#endif
        auto left_fn = left_denotation(current_token(), left);
        if (!left_fn)
            NAT_UNREACHABLE();

        left = (this->*left_fn)(env, left, locals);
    }

    if (left->is_callable() && !current_token().is_end_of_expression() && get_precedence(left) == LOWEST && precedence == LOWEST && !current_token().is_closing_token()) {
        left = parse_call_expression_without_parens(env, left, locals);
    }

    return left;
}

Node *Parser::tree(Env *env) {
    auto tree = new BlockNode { current_token() };
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

BlockNode *Parser::parse_body(Env *env, LocalsVectorPtr locals, Precedence precedence, Token::Type end_token_type) {
    auto body = new BlockNode { current_token() };
    current_token().validate(env);
    skip_newlines();
    while (!current_token().is_eof() && current_token().type() != end_token_type) {
        auto exp = parse_expression(env, precedence, locals);
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
    return body;
}

BlockNode *Parser::parse_body(Env *env, LocalsVectorPtr locals, Precedence precedence, Vector<Token::Type> *end_tokens, const char *expected_message) {
    auto body = new BlockNode { current_token() };
    current_token().validate(env);
    skip_newlines();
    auto finished = [this, end_tokens] {
        for (auto end_token : *end_tokens) {
            if (current_token().type() == end_token)
                return true;
        }
        return false;
    };
    while (!current_token().is_eof() && !finished()) {
        auto exp = parse_expression(env, precedence, locals);
        body->add_node(exp);
        current_token().validate(env);
        next_expression(env);
    }
    if (!finished())
        raise_unexpected(env, expected_message);
    return body;
}

Node *Parser::parse_alias(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    auto new_name = parse_alias_arg(env, locals, "alias new name (first argument)");
    auto existing_name = parse_alias_arg(env, locals, "alias existing name (second argument)");
    return new AliasNode { token, new_name, existing_name };
}

SymbolNode *Parser::parse_alias_arg(Env *env, LocalsVectorPtr locals, const char *expected_message) {
    switch (current_token().type()) {
    case Token::Type::BareName: {
        auto identifier = static_cast<IdentifierNode *>(parse_identifier(env, locals));
        return new SymbolNode { current_token(), SymbolValue::intern(env, identifier->name()) };
    }
    case Token::Type::Symbol:
        return static_cast<SymbolNode *>(parse_symbol(env, locals));
    default:
        raise_unexpected(env, expected_message);
    }
}

Node *Parser::parse_array(Env *env, LocalsVectorPtr locals) {
    auto array = new ArrayNode { current_token() };
    advance();
    if (current_token().type() != Token::Type::RBracket) {
        array->add_node(parse_expression(env, ARRAY, locals));
        while (current_token().type() == Token::Type::Comma) {
            advance();
            if (current_token().type() == Token::Type::RBracket)
                break;
            array->add_node(parse_expression(env, ARRAY, locals));
        }
    }
    expect(env, Token::Type::RBracket, "array closing bracket");
    advance();
    return array;
}

void Parser::parse_comma_separated_expressions(Env *env, ArrayNode *array, LocalsVectorPtr locals) {
    array->add_node(parse_expression(env, ARRAY, locals));
    while (current_token().type() == Token::Type::Comma) {
        advance();
        array->add_node(parse_expression(env, ARRAY, locals));
    }
}

Node *Parser::parse_begin(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    next_expression(env);
    auto begin_tokens = new Vector<Token::Type> { { Token::Type::RescueKeyword, Token::Type::ElseKeyword, Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    auto else_tokens = new Vector<Token::Type> { { Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    auto body = parse_body(env, locals, LOWEST, begin_tokens, "case: rescue, else, ensure, or end");
    auto begin_node = new BeginNode { token, body };
    while (!current_token().is_eof() && !current_token().is_end_keyword()) {
        switch (current_token().type()) {
        case Token::Type::RescueKeyword: {
            auto rescue_node = new BeginRescueNode { current_token() };
            advance();
            if (!current_token().is_eol() && current_token().type() != Token::Type::HashRocket) {
                auto name = parse_expression(env, CALLARGS, locals);
                rescue_node->add_exception_node(name);
                while (current_token().is_comma()) {
                    advance();
                    auto name = parse_expression(env, CALLARGS, locals);
                    rescue_node->add_exception_node(name);
                }
            }
            if (current_token().type() == Token::Type::HashRocket) {
                advance();
                auto name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
                rescue_node->set_exception_name(name);
            }
            next_expression(env);
            auto body = parse_body(env, locals, LOWEST, begin_tokens, "case: rescue, else, ensure, or end");
            rescue_node->set_body(body);
            begin_node->add_rescue_node(rescue_node);
            break;
        }
        case Token::Type::ElseKeyword: {
            advance();
            next_expression(env);
            auto body = parse_body(env, locals, LOWEST, else_tokens, "case: ensure or end");
            begin_node->set_else_body(body);
            break;
        }
        case Token::Type::EnsureKeyword: {
            advance();
            next_expression(env);
            auto body = parse_body(env, locals, LOWEST);
            begin_node->set_ensure_body(body);
            break;
        }
        default:
            raise_unexpected(env, "begin end");
        }
    }
    expect(env, Token::Type::EndKeyword, "case end");
    advance();
    token = current_token();
    switch (token.type()) {
    case Token::Type::UntilKeyword: {
        advance();
        auto condition = parse_expression(env, LOWEST, locals);
        BlockNode *body;
        if (begin_node->no_rescue_nodes() && !begin_node->has_ensure_body())
            body = begin_node->body();
        else
            body = new BlockNode { token, begin_node };
        return new UntilNode { token, condition, body, false };
    }
    case Token::Type::WhileKeyword: {
        advance();
        auto condition = parse_expression(env, LOWEST, locals);
        BlockNode *body;
        if (begin_node->no_rescue_nodes() && !begin_node->has_ensure_body())
            body = begin_node->body();
        else
            body = new BlockNode { token, begin_node };
        return new WhileNode { token, condition, body, false };
    }
    default:
        return begin_node;
    }
}

Node *Parser::parse_block_pass(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    switch (current_token().type()) {
    case Token::Type::BareName:
    case Token::Type::ClassVariable:
    case Token::Type::Constant:
    case Token::Type::GlobalVariable:
    case Token::Type::InstanceVariable:
        return new BlockPassNode { token, parse_expression(env, LOWEST, locals) };
    case Token::Type::Symbol:
        return new BlockPassNode { token, parse_symbol(env, locals) };
    default:
        expect(env, Token::Type::BareName, "block");
    }
    NAT_UNREACHABLE();
}

Node *Parser::parse_bool(Env *env, LocalsVectorPtr) {
    auto token = current_token();
    switch (current_token().type()) {
    case Token::Type::TrueKeyword:
        advance();
        return new TrueNode { token };
    case Token::Type::FalseKeyword:
        advance();
        return new FalseNode { token };
    default:
        NAT_UNREACHABLE();
    }
}

Node *Parser::parse_break(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    if (current_token().is_lparen()) {
        advance();
        auto arg = parse_expression(env, CALLARGS, locals);
        expect(env, Token::Type::RParen, "break closing paren");
        advance();
        return new BreakNode { token, arg };
    } else if (!current_token().is_end_of_expression()) {
        auto array = new ArrayNode { token };
        parse_comma_separated_expressions(env, array, locals);
        return new BreakNode { token, array };
    }
    return new BreakNode { token };
}

Node *Parser::parse_case(Env *env, LocalsVectorPtr locals) {
    auto case_token = current_token();
    advance();
    auto subject = parse_expression(env, CASE, locals);
    next_expression(env);
    auto node = new CaseNode { case_token, subject };
    while (!current_token().is_end_keyword()) {
        auto token = current_token();
        switch (token.type()) {
        case Token::Type::WhenKeyword: {
            advance();
            auto condition_array = new ArrayNode { token };
            parse_comma_separated_expressions(env, condition_array, locals);
            next_expression(env);
            auto body = parse_case_when_body(env, locals);
            auto when_node = new CaseWhenNode { token, condition_array, body };
            node->add_when_node(when_node);
            break;
        }
        case Token::Type::ElseKeyword: {
            advance();
            next_expression(env);
            BlockNode *body = parse_body(env, locals, LOWEST);
            node->set_else_node(body);
            expect(env, Token::Type::EndKeyword, "case end");
            break;
        }
        default:
            raise_unexpected(env, "case when keyword");
        }
    }
    expect(env, Token::Type::EndKeyword, "case end");
    advance();
    return node;
}

BlockNode *Parser::parse_case_when_body(Env *env, LocalsVectorPtr locals) {
    auto body = new BlockNode { current_token() };
    current_token().validate(env);
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_when_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword()) {
        auto exp = parse_expression(env, LOWEST, locals);
        body->add_node(exp);
        current_token().validate(env);
        next_expression(env);
    }
    if (!current_token().is_when_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword())
        raise_unexpected(env, "case: when, else, or end");
    return body;
}

Node *Parser::parse_class(Env *env, LocalsVectorPtr) {
    auto token = current_token();
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
        superclass = new NilNode { token };
    }
    auto body = parse_body(env, locals, LOWEST);
    expect(env, Token::Type::EndKeyword, "class end");
    advance();
    return new ClassNode { token, name, superclass, body };
};

Node *Parser::parse_comma_separated_identifiers(Env *env, LocalsVectorPtr locals) {
    auto list = new MultipleAssignmentNode { current_token() };
    list->add_node(parse_identifier(env, locals));
    while (current_token().is_comma()) {
        advance();
        switch (current_token().type()) {
        case Token::Type::BareName:
        case Token::Type::ClassVariable:
        case Token::Type::Constant:
        case Token::Type::GlobalVariable:
        case Token::Type::InstanceVariable:
            list->add_node(parse_identifier(env, locals));
            break;
        case Token::Type::LParen:
            advance();
            list->add_node(parse_comma_separated_identifiers(env, locals));
            expect(env, Token::Type::RParen, "multiple assignment closing paren");
            advance();
            break;
        default:
            expect(env, Token::Type::BareName, "assignment identifier");
        }
    }
    return list;
}

Node *Parser::parse_constant(Env *env, LocalsVectorPtr locals) {
    auto node = new ConstantNode { current_token() };
    advance();
    return node;
};

Node *Parser::parse_def(Env *env, LocalsVectorPtr) {
    auto token = current_token();
    advance();
    auto locals = new Vector<SymbolValue *> {};
    Node *self_node = nullptr;
    IdentifierNode *name;
    switch (current_token().type()) {
    case Token::Type::BareName:
        if (peek_token().type() == Token::Type::Dot) {
            self_node = parse_identifier(env, locals);
            advance();
            expect(env, Token::Type::BareName, "def name");
            name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
        } else {
            name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
        }
        break;
    case Token::Type::SelfKeyword:
        advance();
        self_node = new SelfNode { current_token() };
        expect(env, Token::Type::Dot, "def obj dot");
        advance();
        expect(env, Token::Type::BareName, "def name");
        name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
        break;
    default:
        raise_unexpected(env, "method name");
    }
    if (current_token().type() == Token::Type::Equal && !current_token().whitespace_precedes()) {
        advance();
        name->append_to_name('=');
    }
    Vector<Node *> *args;
    if (current_token().is_lparen()) {
        advance();
        args = parse_def_args(env, locals);
        expect(env, Token::Type::RParen, "args closing paren");
        advance();
    } else if (current_token().is_bare_name()) {
        args = parse_def_args(env, locals);
    } else {
        args = new Vector<Node *> {};
    }
    auto body = parse_body(env, locals, LOWEST);
    expect(env, Token::Type::EndKeyword, "def end");
    advance();
    return new DefNode { token, self_node, name, args, body };
};

Vector<Node *> *Parser::parse_def_args(Env *env, LocalsVectorPtr locals) {
    auto args = new Vector<Node *> {};
    args->push(parse_def_single_arg(env, locals));
    while (current_token().is_comma()) {
        advance();
        args->push(parse_def_single_arg(env, locals));
    }
    return args;
}

Node *Parser::parse_def_single_arg(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName: {
        auto arg = new ArgNode { token, token.literal() };
        advance();
        locals->push(SymbolValue::intern(env, arg->name()));
        if (current_token().type() == Token::Type::Equal) {
            advance();
            arg->set_value(parse_expression(env, DEFARGS, locals));
        }
        return arg;
    }
    case Token::Type::LParen: {
        advance();
        auto sub_args = parse_def_args(env, locals);
        expect(env, Token::Type::RParen, "nested args closing paren");
        advance();
        auto masgn = new MultipleAssignmentNode { token };
        for (auto arg : *sub_args) {
            masgn->add_node(arg);
        }
        return masgn;
    }
    case Token::Type::Multiply: {
        advance();
        ArgNode *arg;
        if (current_token().is_bare_name()) {
            arg = new ArgNode { token, current_token().literal() };
            advance();
            locals->push(SymbolValue::intern(env, arg->name()));
        } else {
            arg = new ArgNode { token };
        }
        arg->set_splat(true);
        return arg;
    }
    case Token::Type::BitwiseAnd: {
        advance();
        expect(env, Token::Type::BareName, "block name");
        auto arg = new ArgNode { token, current_token().literal() };
        advance();
        locals->push(SymbolValue::intern(env, arg->name()));
        arg->set_block_arg(true);
        return arg;
    }
    case Token::Type::SymbolKey: {
        auto arg = new KeywordArgNode { token, current_token().literal() };
        advance();
        switch (current_token().type()) {
        case Token::Type::Comma:
        case Token::Type::RParen:
        case Token::Type::Eol:
        case Token::Type::BitwiseOr:
            break;
        default:
            arg->set_value(parse_expression(env, DEFARGS, locals));
        }
        locals->push(SymbolValue::intern(env, arg->name()));
        return arg;
    }
    default:
        raise_unexpected(env, "argument");
    }
}

Node *Parser::parse_modifier_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::IfKeyword: {
        advance();
        auto condition = parse_expression(env, LOWEST, locals);
        return new IfNode { token, condition, left, new NilNode { token } };
    }
    case Token::Type::UnlessKeyword: {
        advance();
        auto condition = parse_expression(env, LOWEST, locals);
        return new IfNode { token, condition, new NilNode { token }, left };
    }
    default:
        NAT_NOT_YET_IMPLEMENTED();
    }
}

Node *Parser::parse_file_constant(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    return new StringNode { token, new StringValue { env, token.file() } };
}

Node *Parser::parse_group(Env *env, LocalsVectorPtr locals) {
    advance();
    auto exp = parse_expression(env, LOWEST, locals);
    expect(env, Token::Type::RParen, "group closing paren");
    advance();
    return exp;
};

Node *Parser::parse_hash(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    auto hash = new HashNode { token };
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
            if (current_token().type() == Token::Type::RCurlyBrace)
                break;
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

Node *Parser::parse_identifier(Env *env, LocalsVectorPtr locals) {
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

Node *Parser::parse_if(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(env, LOWEST, locals);
    next_expression(env);
    auto true_expr = parse_if_body(env, locals);
    Node *false_expr;
    if (current_token().is_elsif_keyword()) {
        false_expr = parse_if(env, locals);
        return new IfNode { current_token(), condition, true_expr, false_expr };
    } else {
        if (current_token().is_else_keyword()) {
            advance();
            false_expr = parse_if_body(env, locals);
        } else {
            false_expr = new NilNode { current_token() };
        }
        expect(env, Token::Type::EndKeyword, "if end");
        advance();
        return new IfNode { token, condition, true_expr, false_expr };
    }
}

Node *Parser::parse_if_body(Env *env, LocalsVectorPtr locals) {
    auto body = new BlockNode { current_token() };
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

Node *Parser::parse_interpolated_shell(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    if (current_token().type() == Token::Type::InterpolatedShellEnd) {
        auto shell = new ShellNode { token, new StringValue { env } };
        advance();
        return shell;
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedShellEnd) {
        auto shell = new ShellNode { token, new StringValue { env, current_token().literal() } };
        advance();
        advance();
        return shell;
    } else {
        auto interpolated_shell = new InterpolatedShellNode { token };
        while (current_token().type() != Token::Type::InterpolatedShellEnd) {
            switch (current_token().type()) {
            case Token::Type::EvaluateToStringBegin: {
                advance();
                auto block = new BlockNode { current_token() };
                while (current_token().type() != Token::Type::EvaluateToStringEnd) {
                    block->add_node(parse_expression(env, LOWEST, locals));
                    skip_newlines();
                }
                advance();
                if (block->has_one_node())
                    interpolated_shell->add_node(new EvaluateToStringNode { current_token(), (*block->nodes())[0] });
                else
                    interpolated_shell->add_node(new EvaluateToStringNode { current_token(), block });
                break;
            }
            case Token::Type::String:
                interpolated_shell->add_node(new StringNode { current_token(), new StringValue { env, current_token().literal() } });
                advance();
                break;
            default:
                NAT_UNREACHABLE();
            }
        }
        advance();
        return interpolated_shell;
    }
};

Node *Parser::parse_interpolated_string(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    if (current_token().type() == Token::Type::InterpolatedStringEnd) {
        auto string = new StringNode { token, new StringValue { env } };
        advance();
        return string;
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedStringEnd) {
        auto string = new StringNode { token, new StringValue { env, current_token().literal() } };
        advance();
        advance();
        return string;
    } else {
        auto interpolated_string = new InterpolatedStringNode { token };
        while (current_token().type() != Token::Type::InterpolatedStringEnd) {
            switch (current_token().type()) {
            case Token::Type::EvaluateToStringBegin: {
                advance();
                auto block = new BlockNode { current_token() };
                while (current_token().type() != Token::Type::EvaluateToStringEnd) {
                    block->add_node(parse_expression(env, LOWEST, locals));
                    skip_newlines();
                }
                advance();
                if (block->has_one_node())
                    interpolated_string->add_node(new EvaluateToStringNode { current_token(), (*block->nodes())[0] });
                else
                    interpolated_string->add_node(new EvaluateToStringNode { current_token(), block });
                break;
            }
            case Token::Type::String:
                interpolated_string->add_node(new StringNode { current_token(), new StringValue { env, current_token().literal() } });
                advance();
                break;
            default:
                NAT_UNREACHABLE();
            }
        }
        advance();
        return interpolated_string;
    }
};

Node *Parser::parse_lit(Env *env, LocalsVectorPtr locals) {
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
    return new LiteralNode { token, value };
};

Node *Parser::parse_keyword_args(Env *env, LocalsVectorPtr locals) {
    auto hash = new HashNode { current_token() };
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
    return hash;
}

Node *Parser::parse_module(Env *env, LocalsVectorPtr) {
    auto token = current_token();
    advance();
    auto locals = new Vector<SymbolValue *> {};
    if (current_token().type() != Token::Type::Constant)
        env->raise("SyntaxError", "class/module name must be CONSTANT");
    auto name = static_cast<ConstantNode *>(parse_constant(env, locals));
    auto body = parse_body(env, locals, LOWEST);
    expect(env, Token::Type::EndKeyword, "module end");
    advance();
    return new ModuleNode { token, name, body };
};

Node *Parser::parse_next(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    if (current_token().is_lparen()) {
        advance();
        auto arg = parse_expression(env, CALLARGS, locals);
        expect(env, Token::Type::RParen, "break closing paren");
        advance();
        return new NextNode { token, arg };
    } else if (!current_token().is_end_of_expression()) {
        auto array = new ArrayNode { token };
        parse_comma_separated_expressions(env, array, locals);
        return new NextNode { token, array };
    }
    return new NextNode { token };
}

Node *Parser::parse_nil(Env *env, LocalsVectorPtr) {
    auto token = current_token();
    advance();
    return new NilSexpNode { token };
}

Node *Parser::parse_not(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    auto precedence = get_precedence();
    advance();
    auto node = new CallNode {
        token,
        parse_expression(env, precedence, locals),
        GC_STRDUP("!")
    };
    return node;
}

Node *Parser::parse_regexp(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    auto regexp = new RegexpNode { token, new RegexpValue { env, token.literal() } };
    advance();
    return regexp;
};

Node *Parser::parse_return(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    if (current_token().is_end_of_expression())
        return new ReturnNode { token };
    return new ReturnNode { token, parse_expression(env, CALLARGS, locals) };
};

Node *Parser::parse_self(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    return new SelfNode { token };
};

Node *Parser::parse_splat(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    return new SplatNode { token, parse_expression(env, LOWEST, locals) };
};

Node *Parser::parse_stabby_proc(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    Vector<Node *> *args;
    if (current_token().is_lparen()) {
        advance();
        args = parse_def_args(env, locals);
        expect(env, Token::Type::RParen, "proc args closing paren");
        advance();
    } else if (current_token().is_bare_name()) {
        args = parse_def_args(env, locals);
    } else {
        args = new Vector<Node *> {};
    }
    if (current_token().type() != Token::Type::DoKeyword && current_token().type() != Token::Type::LCurlyBrace)
        raise_unexpected(env, "block");
    return new StabbyProcNode { token, args };
};

Node *Parser::parse_string(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    auto string = new StringNode { token, new StringValue { env, token.literal() } };
    advance();
    return string;
};

Node *Parser::parse_symbol(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    auto symbol = new SymbolNode { token, SymbolValue::intern(env, current_token().literal()) };
    advance();
    return symbol;
};

Node *Parser::parse_top_level_constant(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    const char *name;
    auto name_token = current_token();
    auto identifier = static_cast<IdentifierNode *>(parse_identifier(env, locals));
    switch (identifier->token_type()) {
    case Token::Type::BareName:
    case Token::Type::Constant:
        name = identifier->name();
        break;
    default:
        raise_unexpected(env, name_token, ":: identifier name");
    }
    return new Colon3Node { token, name };
}

Node *Parser::parse_word_array(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    auto array = new ArrayNode { token };
    auto literal = token.literal();
    size_t len = strlen(literal);
    if (len > 0) {
        StringValue *string = new StringValue { env };
        for (size_t i = 0; i < len; i++) {
            auto c = literal[i];
            switch (c) {
            case ' ':
                array->add_node(new StringNode { token, string });
                string = new StringValue { env };
                break;
            default:
                string->append_char(env, c);
            }
        }
        array->add_node(new StringNode { token, string });
    }
    advance();
    return array;
}

Node *Parser::parse_word_symbol_array(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    auto array = new ArrayNode { token };
    auto literal = token.literal();
    size_t len = strlen(literal);
    if (len > 0) {
        StringValue *string = new StringValue { env };
        for (size_t i = 0; i < len; i++) {
            auto c = literal[i];
            switch (c) {
            case ' ':
                array->add_node(new LiteralNode { token, SymbolValue::intern(env, string->c_str()) });
                string = new StringValue { env };
                break;
            default:
                string->append_char(env, c);
            }
        }
        array->add_node(new LiteralNode { token, SymbolValue::intern(env, string->c_str()) });
    }
    advance();
    return array;
}

Node *Parser::parse_yield(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    auto node = new YieldNode { token };
    if (current_token().is_lparen()) {
        advance();
        parse_call_args(env, node, locals);
        expect(env, Token::Type::RParen, "yield closing paren");
        advance();
    } else if (!current_token().is_end_of_expression()) {
        parse_call_args(env, node, locals);
    }
    return node;
};

Node *Parser::parse_assignment_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    switch (left->type()) {
    case Node::Type::Identifier: {
        auto left_identifier = static_cast<IdentifierNode *>(left);
        if (left_identifier->token_type() == Token::Type::BareName)
            locals->push(SymbolValue::intern(env, left_identifier->name()));
        advance();
        return new AssignmentNode {
            token,
            left,
            parse_expression(env, ASSIGNMENT, locals),
        };
    }
    case Node::Type::MultipleAssignment: {
        static_cast<MultipleAssignmentNode *>(left)->add_locals(env, locals);
        advance();
        return new AssignmentNode {
            token,
            left,
            parse_expression(env, ASSIGNMENT, locals),
        };
    }
    case Node::Type::Call: {
        advance();
        auto attr_assign_node = new AttrAssignNode { token, *static_cast<CallNode *>(left) };
        if (strcmp(attr_assign_node->message(), "[]") == 0) {
            attr_assign_node->set_message(GC_STRDUP("[]="));
            attr_assign_node->add_arg(parse_expression(env, ASSIGNMENT, locals));
        } else {
            auto message = std::string(attr_assign_node->message());
            message += '=';
            attr_assign_node->set_message(GC_STRDUP(message.c_str()));
            attr_assign_node->add_arg(parse_expression(env, ASSIGNMENT, locals));
        }
        return attr_assign_node;
    }
    default:
        raise_unexpected(env, left->token(), "left side of assignment");
    }
};

Node *Parser::parse_iter_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    locals = new Vector<SymbolValue *> { *locals };
    bool curly_brace = current_token().type() == Token::Type::LCurlyBrace;
    advance();
    Vector<Node *> *args;
    switch (left->type()) {
    case Node::Type::Identifier:
    case Node::Type::Call:
        if (current_token().type() == Token::Type::BitwiseOr) {
            advance();
            args = parse_iter_args(env, locals);
            expect(env, Token::Type::BitwiseOr, "end of block args");
            advance();
        } else {
            args = new Vector<Node *> {};
        }
        break;
    case Node::Type::StabbyProc:
        args = static_cast<StabbyProcNode *>(left)->args();
        break;
    default:
        raise_unexpected(env, "call");
    }
    auto end_token_type = curly_brace ? Token::Type::RCurlyBrace : Token::Type::EndKeyword;
    auto body = parse_body(env, locals, LOWEST, end_token_type);
    expect(env, end_token_type, "block end");
    advance();
    return new IterNode { token, left, args, body };
}

Vector<Node *> *Parser::parse_iter_args(Env *env, LocalsVectorPtr locals) {
    return parse_def_args(env, locals);
}

Node *Parser::parse_call_expression_with_parens(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    CallNode *call_node;
    switch (left->type()) {
    case Node::Type::Identifier:
        call_node = new CallNode {
            token,
            new NilNode { token },
            GC_STRDUP(static_cast<IdentifierNode *>(left)->name()),
        };
        break;
    case Node::Type::Call:
        call_node = static_cast<CallNode *>(left);
        break;
    case Node::Type::SafeCall:
        call_node = static_cast<SafeCallNode *>(left);
        break;
    default:
        NAT_UNREACHABLE();
    }
    advance();
    if (!current_token().is_rparen())
        parse_call_args(env, call_node, locals);
    expect(env, Token::Type::RParen, "call rparen");
    advance();
    return call_node;
}

void Parser::parse_call_args(Env *env, NodeWithArgs *node, LocalsVectorPtr locals) {
    auto arg = parse_expression(env, CALLARGS, locals);
    node->add_arg(arg);
    while (current_token().is_comma()) {
        advance();
        auto token = current_token();
        if ((token.type() == Token::Type::Symbol && peek_token().type() == Token::Type::HashRocket) || token.type() == Token::Type::SymbolKey) {
            auto hash = parse_keyword_args(env, locals);
            node->add_arg(hash);
            break;
        } else {
            auto arg = parse_expression(env, CALLARGS, locals);
            node->add_arg(arg);
        }
    }
}

Node *Parser::parse_call_expression_without_parens(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    CallNode *call_node;
    switch (left->type()) {
    case Node::Type::Identifier:
        call_node = new CallNode {
            token,
            new NilNode { token },
            GC_STRDUP(static_cast<IdentifierNode *>(left)->name()),
        };
        break;
    case Node::Type::Call:
        call_node = static_cast<CallNode *>(left);
        break;
    case Node::Type::SafeCall:
        call_node = static_cast<SafeCallNode *>(left);
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
        parse_call_args(env, call_node, locals);
    }
    return call_node;
}

Node *Parser::parse_constant_resolution_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    auto name_token = current_token();
    auto identifier = static_cast<IdentifierNode *>(parse_identifier(env, locals));
    switch (identifier->token_type()) {
    case Token::Type::BareName: {
        const char *name = identifier->name();
        return new CallNode { token, left, name };
    }
    case Token::Type::Constant: {
        const char *name = identifier->name();
        return new Colon2Node { token, left, name };
    }
    default:
        raise_unexpected(env, name_token, ":: identifier name");
    }
}

Node *Parser::parse_infix_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    auto op = current_token();
    auto precedence = get_precedence(left);
    advance();
    Node *right = nullptr;
    if (op.type() == Token::Type::Integer) {
        bool is_negative = op.get_integer() < 0;
        right = new LiteralNode { token, new IntegerValue { env, op.get_integer() * (is_negative ? -1 : 1) } };
        op = Token { is_negative ? Token::Type::Minus : Token::Type::Plus, op.file(), op.line(), op.column() };
    } else if (op.type() == Token::Type::Float) {
        bool is_negative = op.get_double() < 0;
        right = new LiteralNode { token, new FloatValue { env, op.get_double() * (is_negative ? -1 : 1) } };
        op = Token { is_negative ? Token::Type::Minus : Token::Type::Plus, op.file(), op.line(), op.column() };
    } else {
        right = parse_expression(env, precedence, locals);
    }
    auto *node = new CallNode {
        token,
        left,
        GC_STRDUP(op.type_value(env)),
    };
    node->add_arg(right);
    return node;
};

Node *Parser::parse_logical_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    switch (current_token().type()) {
    case Token::Type::And:
        advance();
        return new LogicalAndNode { token, left, parse_expression(env, LOGICALAND, locals) };
    case Token::Type::AndKeyword:
        advance();
        return new LogicalAndNode { token, left, parse_expression(env, COMPOSITION, locals) };
    case Token::Type::Or:
        advance();
        return new LogicalOrNode { token, left, parse_expression(env, LOGICALOR, locals) };
    case Token::Type::OrKeyword:
        advance();
        return new LogicalOrNode { token, left, parse_expression(env, COMPOSITION, locals) };
    default:
        NAT_UNREACHABLE();
    }
}

Node *Parser::parse_not_match_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    auto expression = parse_infix_expression(env, left, locals);
    assert(expression->type() == Node::Type::Call);
    static_cast<CallNode *>(expression)->set_message(GC_STRDUP("=~"));
    return new NotNode { token, expression };
}

Node *Parser::parse_op_assign_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    if (left->type() != Node::Type::Identifier)
        raise_unexpected(env, "identifier");
    auto left_identifier = static_cast<IdentifierNode *>(left);
    left_identifier->set_is_lvar(true);
    auto token = current_token();
    advance();
    switch (token.type()) {
    case Token::Type::AndEqual:
        return new OpAssignAndNode { token, left_identifier, parse_expression(env, ASSIGNMENT, locals) };
    case Token::Type::OrEqual:
        return new OpAssignOrNode { token, left_identifier, parse_expression(env, ASSIGNMENT, locals) };
    default:
        NAT_UNREACHABLE();
    }
}

Node *Parser::parse_range_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    return new RangeNode { token, left, parse_expression(env, LOWEST, locals), token.type() == Token::Type::DotDotDot };
}

Node *Parser::parse_ref_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    auto call_node = new CallNode {
        token,
        left,
        GC_STRDUP("[]"),
    };
    if (current_token().type() != Token::Type::RBracket)
        parse_call_args(env, call_node, locals);
    expect(env, Token::Type::RBracket, "element reference right bracket");
    advance();
    return call_node;
}

Node *Parser::parse_safe_send_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    expect(env, Token::Type::BareName, "safe navigator method name");
    auto name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
    auto call_node = new SafeCallNode {
        token,
        left,
        GC_STRDUP(name->name()),
    };
    return call_node;
}

Node *Parser::parse_send_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    expect(env, Token::Type::BareName, "send method name");
    auto name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
    auto call_node = new CallNode {
        token,
        left,
        GC_STRDUP(name->name()),
    };
    return call_node;
}

Node *Parser::parse_ternary_expression(Env *env, Node *left, LocalsVectorPtr locals) {
    auto token = current_token();
    expect(env, Token::Type::TernaryQuestion, "ternary question");
    advance();
    auto true_expr = parse_expression(env, TERNARY, locals);
    expect(env, Token::Type::TernaryColon, "ternary colon");
    advance();
    auto false_expr = parse_expression(env, TERNARY, locals);
    return new IfNode { token, left, true_expr, false_expr };
}

Node *Parser::parse_unless(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(env, LOWEST, locals);
    next_expression(env);
    auto false_expr = parse_if_body(env, locals);
    Node *true_expr;
    if (current_token().is_else_keyword()) {
        advance();
        true_expr = parse_if_body(env, locals);
    } else {
        true_expr = new NilNode { current_token() };
    }
    expect(env, Token::Type::EndKeyword, "if end");
    advance();
    return new IfNode { token, condition, true_expr, false_expr };
}

Node *Parser::parse_while(Env *env, LocalsVectorPtr locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(env, LOWEST, locals);
    next_expression(env);
    auto body = parse_body(env, locals, LOWEST);
    expect(env, Token::Type::EndKeyword, "while end");
    advance();
    switch (token.type()) {
    case Token::Type::UntilKeyword:
        return new UntilNode { token, condition, body, true };
    case Token::Type::WhileKeyword:
        return new WhileNode { token, condition, body, true };
    default:
        NAT_UNREACHABLE();
    }
}

Parser::parse_null_fn Parser::null_denotation(Token::Type type, Precedence precedence) {
    using Type = Token::Type;
    switch (type) {
    case Type::AliasKeyword:
        return &Parser::parse_alias;
    case Type::LBracket:
        return &Parser::parse_array;
    case Type::BeginKeyword:
        return &Parser::parse_begin;
    case Type::BitwiseAnd:
        return &Parser::parse_block_pass;
    case Type::TrueKeyword:
    case Type::FalseKeyword:
        return &Parser::parse_bool;
    case Type::BreakKeyword:
        return &Parser::parse_break;
    case Type::CaseKeyword:
        return &Parser::parse_case;
    case Type::ClassKeyword:
        return &Parser::parse_class;
    case Type::DefKeyword:
        return &Parser::parse_def;
    case Type::FILEKeyword:
        return &Parser::parse_file_constant;
    case Type::LParen:
        return &Parser::parse_group;
    case Type::LCurlyBrace:
        return &Parser::parse_hash;
    case Type::BareName:
    case Type::ClassVariable:
    case Type::Constant:
    case Type::GlobalVariable:
    case Type::InstanceVariable:
        if (peek_token().is_comma() && precedence == LOWEST) {
            return &Parser::parse_comma_separated_identifiers;
        } else {
            return &Parser::parse_identifier;
        }
    case Type::IfKeyword:
        return &Parser::parse_if;
    case Type::InterpolatedShellBegin:
        return &Parser::parse_interpolated_shell;
    case Type::InterpolatedStringBegin:
        return &Parser::parse_interpolated_string;
    case Type::Integer:
    case Type::Float:
        return &Parser::parse_lit;
    case Type::ModuleKeyword:
        return &Parser::parse_module;
    case Type::NextKeyword:
        return &Parser::parse_next;
    case Type::NilKeyword:
        return &Parser::parse_nil;
    case Type::Not:
    case Type::NotKeyword:
        return &Parser::parse_not;
    case Type::Regexp:
        return &Parser::parse_regexp;
    case Type::ReturnKeyword:
        return &Parser::parse_return;
    case Type::SelfKeyword:
        return &Parser::parse_self;
    case Type::Multiply:
        return &Parser::parse_splat;
    case Type::Arrow:
        return &Parser::parse_stabby_proc;
    case Type::String:
        return &Parser::parse_string;
    case Type::Symbol:
        return &Parser::parse_symbol;
    case Type::ConstantResolution:
        return &Parser::parse_top_level_constant;
    case Type::UnlessKeyword:
        return &Parser::parse_unless;
    case Type::UntilKeyword:
    case Type::WhileKeyword:
        return &Parser::parse_while;
    case Type::PercentLowerI:
    case Type::PercentUpperI:
        return &Parser::parse_word_symbol_array;
    case Type::PercentLowerW:
    case Type::PercentUpperW:
        return &Parser::parse_word_array;
    case Type::YieldKeyword:
        return &Parser::parse_yield;
    default:
        return nullptr;
    }
};

Parser::parse_left_fn Parser::left_denotation(Token token, Node *left) {
    using Type = Token::Type;
    switch (token.type()) {
    case Type::Equal:
        return &Parser::parse_assignment_expression;
    case Type::LParen:
        return &Parser::parse_call_expression_with_parens;
    case Type::ConstantResolution:
        return &Parser::parse_constant_resolution_expression;
    case Type::BitwiseAnd:
    case Type::BitwiseOr:
    case Type::BitwiseXor:
    case Type::Divide:
    case Type::EqualEqual:
    case Type::EqualEqualEqual:
    case Type::Exponent:
    case Type::GreaterThan:
    case Type::GreaterThanOrEqual:
    case Type::LeftShift:
    case Type::LessThan:
    case Type::LessThanOrEqual:
    case Type::Match:
    case Type::Minus:
    case Type::Modulus:
    case Type::Multiply:
    case Type::NotEqual:
    case Type::Plus:
    case Type::RightShift:
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
    case Type::And:
    case Type::AndKeyword:
    case Type::Or:
    case Type::OrKeyword:
        return &Parser::parse_logical_expression;
    case Type::IfKeyword:
    case Type::UnlessKeyword:
    case Type::WhileKeyword:
    case Type::UntilKeyword:
        return &Parser::parse_modifier_expression;
    case Type::NotMatch:
        return &Parser::parse_not_match_expression;
    case Type::AndEqual:
    case Type::OrEqual:
        return &Parser::parse_op_assign_expression;
    case Type::DotDot:
    case Type::DotDotDot:
        return &Parser::parse_range_expression;
    case Type::LBracket:
        if (treat_left_bracket_as_element_reference(left, current_token()))
            return &Parser::parse_ref_expression;
        else
            return nullptr;
    case Type::SafeNavigation:
        return &Parser::parse_safe_send_expression;
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

void Parser::raise_unexpected(Env *env, Token token, const char *expected) {
    auto file = token.file() ? token.file() : "(unknown)";
    auto line = token.line() + 1;
    auto type = token.type_value(env);
    auto literal = token.literal();
    if (strcmp(type, "EOF") == 0)
        env->raise("SyntaxError", "%s#%d: syntax error, unexpected end-of-input (expected: '%s')", file, line, expected);
    else if (literal)
        env->raise("SyntaxError", "%s#%d: syntax error, unexpected %s '%s' (expected: '%s')", file, line, type, literal, expected);
    else
        env->raise("SyntaxError", "%s#%d: syntax error, unexpected '%s' (expected: '%s')", file, line, type, expected);
}

void Parser::raise_unexpected(Env *env, const char *expected) {
    raise_unexpected(env, current_token(), expected);
}
}
