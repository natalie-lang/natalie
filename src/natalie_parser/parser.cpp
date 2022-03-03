#include "natalie_parser/parser.hpp"

namespace NatalieParser {

enum class Parser::Precedence {
    LOWEST,
    ARRAY, // []
    HASH, // {}
    EXPRMODIFIER, // if/unless/while/until
    CASE, // case/when/else
    ASSIGNMENT, // =
    SPLAT, // *args, **kwargs
    CALLARGS, // foo(a, b)
    COMPOSITION, // and/or
    OPASSIGNMENT, // += -= *= **= /= %= |= &= ^= >>= <<= ||= &&=
    TERNARY, // ? :
    ITER_BLOCK, // do |n| ... end
    BARECALLARGS, // foo a, b
    RANGE, // ..
    ITER_CURLY, // { |n| ... }
    LOGICALNOT, // not
    LOGICALOR, // ||
    LOGICALAND, // &&
    EQUALITY, // <=> == === != =~ !~
    LESSGREATER, // <= < > >=
    BITWISEOR, // ^ |
    BITWISEAND, // &
    BITWISESHIFT, // << >>
    DEFARGS, // def foo(a, b) and { |a, b| ... }
    SUM, // + -
    PRODUCT, // * / %
    PREFIX, // -1 +1
    UNARY_MINUS, // -
    EXPONENT, // **
    UNARY_PLUS, // ! ~ +
    ASSIGNMENTIDENTIFIER,
    CONSTANTRESOLUTION, // ::
    DOT, // foo.bar foo&.bar
    CALL, // foo()
    REF, // foo[1] / foo[1] = 2
};

bool Parser::higher_precedence(Token &token, Node *left, Precedence current_precedence) {
    auto next_precedence = get_precedence(token, left);
    // trick to make chained assignment right-to-left
    if (current_precedence == Precedence::ASSIGNMENT && next_precedence == Precedence::ASSIGNMENT)
        return true;
    return next_precedence > current_precedence;
}

Parser::Precedence Parser::get_precedence(Token &token, Node *left) {
    switch (token.type()) {
    case Token::Type::Plus:
        return left ? Precedence::SUM : Precedence::UNARY_PLUS;
    case Token::Type::Minus:
        return left ? Precedence::SUM : Precedence::UNARY_MINUS;
    case Token::Type::Equal:
        return Precedence::ASSIGNMENT;
    case Token::Type::AndEqual:
    case Token::Type::BitwiseAndEqual:
    case Token::Type::BitwiseOrEqual:
    case Token::Type::BitwiseXorEqual:
    case Token::Type::DivideEqual:
    case Token::Type::ExponentEqual:
    case Token::Type::LeftShiftEqual:
    case Token::Type::MinusEqual:
    case Token::Type::ModulusEqual:
    case Token::Type::MultiplyEqual:
    case Token::Type::OrEqual:
    case Token::Type::PlusEqual:
    case Token::Type::RightShiftEqual:
        return Precedence::OPASSIGNMENT;
    case Token::Type::BitwiseAnd:
        return Precedence::BITWISEAND;
    case Token::Type::BitwiseOr:
    case Token::Type::BitwiseXor:
        return Precedence::BITWISEOR;
    case Token::Type::Comma:
        // NOTE: the only time this precedence is used is for multiple assignment
        return Precedence::ARRAY;
    case Token::Type::LeftShift:
    case Token::Type::RightShift:
        return Precedence::BITWISESHIFT;
    case Token::Type::LParen:
        return Precedence::CALL;
    case Token::Type::AndKeyword:
    case Token::Type::OrKeyword:
        return Precedence::COMPOSITION;
    case Token::Type::ConstantResolution:
        return Precedence::CONSTANTRESOLUTION;
    case Token::Type::Dot:
    case Token::Type::SafeNavigation:
        return Precedence::DOT;
    case Token::Type::EqualEqual:
    case Token::Type::EqualEqualEqual:
    case Token::Type::NotEqual:
    case Token::Type::Match:
    case Token::Type::NotMatch:
        return Precedence::EQUALITY;
    case Token::Type::Exponent:
        return Precedence::EXPONENT;
    case Token::Type::IfKeyword:
    case Token::Type::UnlessKeyword:
    case Token::Type::WhileKeyword:
    case Token::Type::UntilKeyword:
        return Precedence::EXPRMODIFIER;
    case Token::Type::DoKeyword:
        return Precedence::ITER_BLOCK;
    case Token::Type::LCurlyBrace:
        return Precedence::ITER_CURLY;
    case Token::Type::Comparison:
    case Token::Type::LessThan:
    case Token::Type::LessThanOrEqual:
    case Token::Type::GreaterThan:
    case Token::Type::GreaterThanOrEqual:
        return Precedence::LESSGREATER;
    case Token::Type::And:
        return Precedence::LOGICALAND;
    case Token::Type::NotKeyword:
        return Precedence::LOGICALNOT;
    case Token::Type::Or:
        return Precedence::LOGICALOR;
    case Token::Type::Divide:
    case Token::Type::Modulus:
    case Token::Type::Multiply:
        return Precedence::PRODUCT;
    case Token::Type::DotDot:
    case Token::Type::DotDotDot:
        return Precedence::RANGE;
    case Token::Type::LBracket:
    case Token::Type::LBracketRBracket: {
        auto current = current_token();
        if (left && treat_left_bracket_as_element_reference(left, current))
            return Precedence::REF;
        break;
    }
    case Token::Type::TernaryQuestion:
    case Token::Type::TernaryColon:
        return Precedence::TERNARY;
    case Token::Type::Not:
        return Precedence::UNARY_PLUS;
    default:
        break;
    }
    auto current = current_token();
    if (left && is_first_arg_of_call_without_parens(left, current))
        return Precedence::CALL;
    return Precedence::LOWEST;
}

Node *Parser::parse_expression(Parser::Precedence precedence, LocalsHashmap &locals) {
    skip_newlines();

    auto null_fn = null_denotation(current_token().type(), precedence);
    if (!null_fn) {
        throw_unexpected("expression");
    }

    Node *left = (this->*null_fn)(locals);

    while (current_token().is_valid()) {
        auto token = current_token();
        if (!higher_precedence(token, left, precedence))
            break;
        auto left_fn = left_denotation(token, left);
        assert(left_fn);
        left = (this->*left_fn)(left, locals);
    }

    return left;
}

Node *Parser::tree() {
    auto tree = new BlockNode { current_token() };
    current_token().validate();
    LocalsHashmap locals { TM::HashType::String };
    skip_newlines();
    while (!current_token().is_eof()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        tree->add_node(exp);
        current_token().validate();
        next_expression();
    }
    return tree;
}

BlockNode *Parser::parse_body(LocalsHashmap &locals, Precedence precedence, Token::Type end_token_type) {
    auto body = new BlockNode { current_token() };
    current_token().validate();
    skip_newlines();
    while (!current_token().is_eof() && current_token().type() != end_token_type) {
        auto exp = parse_expression(precedence, locals);
        body->add_node(exp);
        current_token().validate();
        if (end_token_type == Token::Type::EndKeyword) {
            next_expression();
        } else {
            auto token = current_token();
            if (token.type() != end_token_type && !token.is_end_of_expression())
                throw_unexpected("end-of-line");
            skip_newlines();
        }
    }
    return body;
}

BlockNode *Parser::parse_body(LocalsHashmap &locals, Precedence precedence, Vector<Token::Type> &end_tokens, const char *expected_message) {
    auto body = new BlockNode { current_token() };
    current_token().validate();
    skip_newlines();
    auto finished = [this, end_tokens] {
        for (auto end_token : end_tokens) {
            if (current_token().type() == end_token)
                return true;
        }
        return false;
    };
    while (!current_token().is_eof() && !finished()) {
        auto exp = parse_expression(precedence, locals);
        body->add_node(exp);
        current_token().validate();
        next_expression();
    }
    if (!finished())
        throw_unexpected(expected_message);
    return body;
}

BlockNode *Parser::parse_def_body(LocalsHashmap &locals) {
    auto token = current_token();
    auto body = new BlockNode { token };
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_end_keyword()) {
        if (current_token().type() == Token::Type::RescueKeyword) {
            auto begin_node = new BeginNode { token, body };
            parse_rest_of_begin(begin_node, locals);
            rewind(); // so the 'end' keyword can be consumed by parse_def
            return new BlockNode { token, begin_node };
        }
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        next_expression();
    }
    return body;
}

Node *Parser::parse_alias(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto new_name = parse_alias_arg(locals, "alias new name (first argument)");
    auto existing_name = parse_alias_arg(locals, "alias existing name (second argument)");
    return new AliasNode { token, new_name, existing_name };
}

SymbolNode *Parser::parse_alias_arg(LocalsHashmap &locals, const char *expected_message) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName: {
        auto identifier = static_cast<IdentifierNode *>(parse_identifier(locals));
        return new SymbolNode { token, identifier->name() };
    }
    case Token::Type::Symbol:
        return static_cast<SymbolNode *>(parse_symbol(locals));
    default:
        if (token.is_operator()) {
            advance();
            if (token.can_precede_collapsible_newline()) {
                // Some operators at the end of a line cause the newlines to be collapsed:
                //
                //     foo <<
                //       bar
                //
                // ...but in this case (an alias), collapsing the newline was a mistake:
                //
                //     alias foo <<
                //     def bar; end
                //
                // So, we'll put the newline back.
                m_tokens->insert(m_index, Token { Token::Type::Eol, token.file(), token.line(), token.column() });
            }
            return new SymbolNode { token, new String(token.type_value()) };
        } else {
            throw_unexpected(expected_message);
        }
    }
}

Node *Parser::parse_array(LocalsHashmap &locals) {
    auto array = new ArrayNode { current_token() };
    if (current_token().type() == Token::Type::LBracketRBracket) {
        advance();
        return array;
    }
    advance();
    if (current_token().type() != Token::Type::RBracket) {
        array->add_node(parse_expression(Precedence::ARRAY, locals));
        while (current_token().is_comma()) {
            advance();
            if (current_token().type() == Token::Type::RBracket)
                break;
            array->add_node(parse_expression(Precedence::ARRAY, locals));
        }
    }
    expect(Token::Type::RBracket, "array closing bracket");
    advance();
    return array;
}

void Parser::parse_comma_separated_expressions(ArrayNode *array, LocalsHashmap &locals) {
    array->add_node(parse_expression(Precedence::ARRAY, locals));
    while (current_token().type() == Token::Type::Comma) {
        advance();
        array->add_node(parse_expression(Precedence::ARRAY, locals));
    }
}

Node *Parser::parse_begin(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    next_expression();
    auto begin_ending_tokens = Vector<Token::Type> { { Token::Type::RescueKeyword, Token::Type::ElseKeyword, Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    auto body = parse_body(locals, Precedence::LOWEST, begin_ending_tokens, "case: rescue, else, ensure, or end");

    auto begin_node = new BeginNode { token, body };
    parse_rest_of_begin(begin_node, locals);

    token = current_token();
    switch (token.type()) {
    case Token::Type::UntilKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        BlockNode *body;
        if (begin_node->no_rescue_nodes() && !begin_node->has_ensure_body())
            body = begin_node->body();
        else
            body = new BlockNode { token, begin_node };
        return new UntilNode { token, condition, body, false };
    }
    case Token::Type::WhileKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
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

void Parser::parse_rest_of_begin(BeginNode *begin_node, LocalsHashmap &locals) {
    auto rescue_ending_tokens = Vector<Token::Type> { { Token::Type::RescueKeyword, Token::Type::ElseKeyword, Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    auto else_ending_tokens = Vector<Token::Type> { { Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    while (!current_token().is_eof() && !current_token().is_end_keyword()) {
        switch (current_token().type()) {
        case Token::Type::RescueKeyword: {
            auto rescue_node = new BeginRescueNode { current_token() };
            advance();
            if (!current_token().is_eol() && current_token().type() != Token::Type::HashRocket) {
                auto name = parse_expression(Precedence::BARECALLARGS, locals);
                rescue_node->add_exception_node(name);
                while (current_token().is_comma()) {
                    advance();
                    auto name = parse_expression(Precedence::BARECALLARGS, locals);
                    rescue_node->add_exception_node(name);
                }
            }
            if (current_token().type() == Token::Type::HashRocket) {
                advance();
                auto name = static_cast<IdentifierNode *>(parse_identifier(locals));
                name->add_to_locals(locals);
                rescue_node->set_exception_name(name);
            }
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST, rescue_ending_tokens, "case: rescue, else, ensure, or end");
            rescue_node->set_body(body);
            begin_node->add_rescue_node(rescue_node);
            break;
        }
        case Token::Type::ElseKeyword: {
            advance();
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST, else_ending_tokens, "case: ensure or end");
            begin_node->set_else_body(body);
            break;
        }
        case Token::Type::EnsureKeyword: {
            advance();
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST);
            begin_node->set_ensure_body(body);
            break;
        }
        default:
            throw_unexpected("begin end");
        }
    }
    expect(Token::Type::EndKeyword, "begin/rescue/ensure end");
    advance();
}

Node *Parser::parse_beginless_range(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new RangeNode { token, new NilNode { token }, parse_expression(Precedence::LOWEST, locals), token.type() == Token::Type::DotDotDot };
}

Node *Parser::parse_block_pass(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    switch (current_token().type()) {
    case Token::Type::BareName:
    case Token::Type::ClassVariable:
    case Token::Type::Constant:
    case Token::Type::GlobalVariable:
    case Token::Type::InstanceVariable:
    case Token::Type::NilKeyword:
        return new BlockPassNode { token, parse_expression(Precedence::LOWEST, locals) };
    case Token::Type::Symbol:
        return new BlockPassNode { token, parse_symbol(locals) };
    default:
        expect(Token::Type::BareName, "block");
    }
    TM_UNREACHABLE();
}

Node *Parser::parse_bool(LocalsHashmap &) {
    auto token = current_token();
    switch (current_token().type()) {
    case Token::Type::TrueKeyword:
        advance();
        return new TrueNode { token };
    case Token::Type::FalseKeyword:
        advance();
        return new FalseNode { token };
    default:
        TM_UNREACHABLE();
    }
}

Node *Parser::parse_break(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
            return new BreakNode { token, new NilSexpNode { token } };
        } else {
            auto arg = parse_expression(Precedence::BARECALLARGS, locals);
            expect(Token::Type::RParen, "break closing paren");
            advance();
            return new BreakNode { token, arg };
        }
    } else if (!current_token().is_end_of_expression()) {
        auto value = parse_expression(Precedence::BARECALLARGS, locals);
        if (current_token().is_comma()) {
            auto array = new ArrayNode { token };
            array->add_node(value);
            while (current_token().is_comma()) {
                advance();
                array->add_node(parse_expression(Precedence::BARECALLARGS, locals));
            }
            value = array;
        }
        return new BreakNode { token, value };
    }
    return new BreakNode { token };
}

Node *Parser::parse_case(LocalsHashmap &locals) {
    auto case_token = current_token();
    advance();
    Node *subject;
    if (current_token().type() == Token::Type::WhenKeyword) {
        subject = new NilNode { case_token };
    } else {
        subject = parse_expression(Precedence::CASE, locals);
        next_expression();
    }
    auto node = new CaseNode { case_token, subject };
    while (!current_token().is_end_keyword()) {
        auto token = current_token();
        switch (token.type()) {
        case Token::Type::WhenKeyword: {
            advance();
            auto condition_array = new ArrayNode { token };
            parse_comma_separated_expressions(condition_array, locals);
            if (current_token().type() == Token::Type::ThenKeyword) {
                advance();
                skip_newlines();
            } else {
                next_expression();
            }
            auto body = parse_case_when_body(locals);
            auto when_node = new CaseWhenNode { token, condition_array, body };
            node->add_node(when_node);
            break;
        }
        case Token::Type::InKeyword: {
            advance();
            auto pattern = parse_case_in_patterns(locals);
            if (current_token().type() == Token::Type::ThenKeyword) {
                advance();
                skip_newlines();
            } else {
                next_expression();
            }
            auto body = parse_case_in_body(locals);
            auto in_node = new CaseInNode { token, pattern, body };
            node->add_node(in_node);
            break;
        }
        case Token::Type::ElseKeyword: {
            advance();
            skip_newlines();
            BlockNode *body = parse_body(locals, Precedence::LOWEST);
            node->set_else_node(body);
            expect(Token::Type::EndKeyword, "case end");
            break;
        }
        default:
            throw_unexpected("case when keyword");
        }
    }
    expect(Token::Type::EndKeyword, "case end");
    advance();
    return node;
}

BlockNode *Parser::parse_case_in_body(LocalsHashmap &locals) {
    return parse_case_when_body(locals);
}

Node *Parser::parse_case_in_pattern(LocalsHashmap &locals) {
    auto token = current_token();
    Node *node;
    switch (token.type()) {
    case Token::Type::BareName:
        advance();
        node = new IdentifierNode { token, true };
        break;
    case Token::Type::BitwiseXor: // caret (^)
        advance();
        expect(Token::Type::BareName, "pinned variable name");
        node = new PinNode { token, new IdentifierNode { current_token(), true } };
        advance();
        break;
    case Token::Type::LBracketRBracket:
        advance();
        node = new ArrayPatternNode { token };
        break;
    case Token::Type::LBracket: {
        // TODO: might need to keep track of and pass along precedence value?
        advance();
        auto array = new ArrayPatternNode { token };
        if (current_token().type() == Token::Type::RBracket) {
            advance();
            node = array;
            break;
        }
        array->add_node(parse_case_in_pattern(locals));
        while (current_token().is_comma()) {
            advance();
            array->add_node(parse_case_in_pattern(locals));
        }
        expect(Token::Type::RBracket, "array pattern closing bracket");
        advance();
        node = array;
        break;
    }
    case Token::Type::LCurlyBrace: {
        advance();
        auto hash = new HashPatternNode { token };
        if (current_token().type() == Token::Type::RCurlyBrace) {
            advance();
            node = hash;
            break;
        }
        expect(Token::Type::SymbolKey, "hash pattern symbol key");
        hash->add_node(parse_symbol(locals));
        hash->add_node(parse_case_in_pattern(locals));
        while (current_token().is_comma()) {
            advance();
            hash->add_node(parse_symbol(locals));
            hash->add_node(parse_case_in_pattern(locals));
        }
        expect(Token::Type::RCurlyBrace, "hash pattern closing brace");
        advance();
        node = hash;
        break;
    }
    case Token::Type::Integer:
    case Token::Type::Float:
        node = parse_lit(locals);
        break;
    case Token::Type::String:
        node = parse_string(locals);
        break;
    case Token::Type::Symbol:
        node = parse_symbol(locals);
        break;
    default:
        printf("TODO: implement token type %d in Parser::parse_case_in_pattern()\n", (int)token.type());
        abort();
    }
    token = current_token();
    if (token.is_hash_rocket()) {
        advance();
        expect(Token::Type::BareName, "pattern name");
        token = current_token();
        advance();
        auto identifier = new IdentifierNode { token, true };
        node = new AssignmentNode { token, identifier, node };
    }
    return node;
}

Node *Parser::parse_case_in_patterns(LocalsHashmap &locals) {
    Vector<Node *> patterns;
    patterns.push(parse_case_in_pattern(locals));
    while (current_token().type() == Token::Type::BitwiseOr) {
        advance();
        patterns.push(parse_case_in_pattern(locals));
    }
    assert(patterns.size() > 0);
    if (patterns.size() == 1) {
        return patterns.first();
    } else {
        auto first = patterns.pop_front();
        auto second = patterns.pop_front();
        auto pattern = new LogicalOrNode { first->token(), first, second };
        while (!patterns.is_empty()) {
            auto next = patterns.pop_front();
            pattern = new LogicalOrNode { next->token(), pattern, next };
        }
        return pattern;
    }
}

BlockNode *Parser::parse_case_when_body(LocalsHashmap &locals) {
    auto body = new BlockNode { current_token() };
    current_token().validate();
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_when_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        current_token().validate();
        next_expression();
    }
    if (!current_token().is_when_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword())
        throw_unexpected("case: when, else, or end");
    return body;
}

Node *Parser::parse_class_or_module_name(LocalsHashmap &locals) {
    Token name_token;
    if (current_token().type() == Token::Type::ConstantResolution) {
        name_token = peek_token();
    } else {
        name_token = current_token();
    }
    if (name_token.type() != Token::Type::Constant)
        throw SyntaxError { "class/module name must be CONSTANT" };
    return parse_expression(Precedence::LESSGREATER, locals);
}

Node *Parser::parse_class(LocalsHashmap &locals) {
    auto token = current_token();
    if (peek_token().type() == Token::Type::LeftShift)
        return parse_sclass(locals);
    advance();
    LocalsHashmap our_locals { TM::HashType::String };
    auto name = parse_class_or_module_name(our_locals);
    Node *superclass;
    if (current_token().type() == Token::Type::LessThan) {
        advance();
        superclass = parse_expression(Precedence::LOWEST, our_locals);
    } else {
        superclass = new NilNode { token };
    }
    auto body = parse_body(our_locals, Precedence::LOWEST);
    expect(Token::Type::EndKeyword, "class end");
    advance();
    return new ClassNode { token, name, superclass, body };
};

Node *Parser::parse_multiple_assignment_expression(Node *left, LocalsHashmap &locals) {
    auto list = new MultipleAssignmentNode { left->token() };
    list->add_node(left);
    while (current_token().is_comma()) {
        advance();
        switch (current_token().type()) {
        case Token::Type::BareName:
        case Token::Type::ClassVariable:
        case Token::Type::Constant:
        case Token::Type::ConstantResolution:
        case Token::Type::GlobalVariable:
        case Token::Type::InstanceVariable:
            list->add_node(parse_expression(Precedence::ASSIGNMENTIDENTIFIER, locals));
            break;
        case Token::Type::LParen:
            list->add_node(parse_group(locals));
            break;
        case Token::Type::Multiply: {
            auto splat_token = current_token();
            advance();
            if (current_token().is_assignable()) {
                auto node = parse_expression(Precedence::ASSIGNMENTIDENTIFIER, locals);
                list->add_node(new SplatNode { splat_token, node });
            } else {
                list->add_node(new SplatNode { splat_token });
            }
            break;
        }
        default:
            expect(Token::Type::BareName, "assignment identifier");
        }
    }
    return list;
}

Node *Parser::parse_constant(LocalsHashmap &locals) {
    auto node = new ConstantNode { current_token() };
    advance();
    return node;
};

Node *Parser::parse_def(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    LocalsHashmap our_locals { TM::HashType::String };
    Node *self_node = nullptr;
    SharedPtr<String> name = new String("");
    token = current_token();
    switch (token.type()) {
    case Token::Type::BareName:
        if (peek_token().type() == Token::Type::Dot) {
            self_node = parse_identifier(locals);
            advance();
        }
        name = parse_method_name(locals);
        break;
    case Token::Type::SelfKeyword:
        advance();
        self_node = new SelfNode { current_token() };
        expect(Token::Type::Dot, "def obj dot");
        advance();
        name = parse_method_name(locals);
        break;
    case Token::Type::Constant:
        if (peek_token().type() == Token::Type::Dot) {
            self_node = parse_constant(locals);
            advance();
        }
        name = parse_method_name(locals);
        break;
    default:
        if (token.is_operator())
            name = parse_method_name(locals);
        else
            throw_unexpected("method name");
    }
    if (current_token().type() == Token::Type::Equal && !current_token().whitespace_precedes()) {
        advance();
        name->append_char('=');
    }
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
        } else {
            args = parse_def_args(our_locals);
            expect(Token::Type::RParen, "args closing paren");
            advance();
        }
    } else if (current_token().is_bare_name() || current_token().is_splat()) {
        args = parse_def_args(our_locals);
    }
    auto body = parse_def_body(our_locals);
    expect(Token::Type::EndKeyword, "def end");
    advance();
    return new DefNode { token, self_node, name, *args, body };
};

Node *Parser::parse_defined(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto arg = parse_expression(Precedence::BARECALLARGS, locals);
    return new DefinedNode { token, arg };
}

SharedPtr<Vector<Node *>> Parser::parse_def_args(LocalsHashmap &locals) {
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    args->push(parse_def_single_arg(locals));
    while (current_token().is_comma()) {
        advance();
        args->push(parse_def_single_arg(locals));
    }
    return args;
}

Node *Parser::parse_def_single_arg(LocalsHashmap &locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName: {
        auto arg = new ArgNode { token, token.literal_string() };
        advance();
        arg->add_to_locals(locals);
        if (current_token().type() == Token::Type::Equal) {
            advance();
            arg->set_value(parse_expression(Precedence::DEFARGS, locals));
        }
        return arg;
    }
    case Token::Type::LParen: {
        advance();
        auto sub_args = parse_def_args(locals);
        expect(Token::Type::RParen, "nested args closing paren");
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
            arg = new ArgNode { token, current_token().literal_string() };
            advance();
            arg->add_to_locals(locals);
        } else {
            arg = new ArgNode { token };
        }
        arg->set_splat(true);
        return arg;
    }
    case Token::Type::Exponent: {
        advance();
        ArgNode *arg;
        if (current_token().is_bare_name()) {
            arg = new ArgNode { token, current_token().literal_string() };
            advance();
            arg->add_to_locals(locals);
        } else {
            arg = new ArgNode { token };
        }
        arg->set_kwsplat(true);
        return arg;
    }
    case Token::Type::BitwiseAnd: {
        advance();
        expect(Token::Type::BareName, "block name");
        auto arg = new ArgNode { token, current_token().literal_string() };
        advance();
        arg->add_to_locals(locals);
        arg->set_block_arg(true);
        return arg;
    }
    case Token::Type::SymbolKey: {
        auto arg = new KeywordArgNode { token, current_token().literal_string() };
        advance();
        switch (current_token().type()) {
        case Token::Type::Comma:
        case Token::Type::RParen:
        case Token::Type::Eol:
        case Token::Type::BitwiseOr:
            break;
        default:
            arg->set_value(parse_expression(Precedence::DEFARGS, locals));
        }
        arg->add_to_locals(locals);
        return arg;
    }
    default:
        throw_unexpected("argument");
    }
}

Node *Parser::parse_modifier_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::IfKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        return new IfNode { token, condition, left, new NilNode { token } };
    }
    case Token::Type::UnlessKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        return new IfNode { token, condition, new NilNode { token }, left };
    }
    case Token::Type::UntilKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        auto body = new BlockNode { token, left };
        return new UntilNode { token, condition, body, true };
    }
    case Token::Type::WhileKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        auto body = new BlockNode { token, left };
        return new WhileNode { token, condition, body, true };
    }
    default:
        TM_NOT_YET_IMPLEMENTED();
    }
}

Node *Parser::parse_file_constant(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new StringNode { token, token.file() };
}

Node *Parser::parse_group(LocalsHashmap &locals) {
    advance();
    auto exp = parse_expression(Precedence::LOWEST, locals);
    expect(Token::Type::RParen, "group closing paren");
    advance();
    return exp;
};

Node *Parser::parse_hash(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto hash = new HashNode { token };
    if (current_token().type() != Token::Type::RCurlyBrace) {
        if (current_token().type() == Token::Type::SymbolKey) {
            hash->add_node(parse_symbol(locals));
        } else {
            hash->add_node(parse_expression(Precedence::HASH, locals));
            expect(Token::Type::HashRocket, "hash rocket");
            advance();
        }
        hash->add_node(parse_expression(Precedence::HASH, locals));
        while (current_token().type() == Token::Type::Comma) {
            advance();
            if (current_token().type() == Token::Type::RCurlyBrace)
                break;
            if (current_token().type() == Token::Type::SymbolKey) {
                hash->add_node(parse_symbol(locals));
            } else {
                hash->add_node(parse_expression(Precedence::HASH, locals));
                expect(Token::Type::HashRocket, "hash rocket");
                advance();
            }
            hash->add_node(parse_expression(Precedence::HASH, locals));
        }
    }
    expect(Token::Type::RCurlyBrace, "hash closing curly brace");
    advance();
    return hash;
}

Node *Parser::parse_identifier(LocalsHashmap &locals) {
    auto name = current_token().literal();
    bool is_lvar = !!locals.get(name);
    auto identifier = new IdentifierNode { current_token(), is_lvar };
    advance();
    return identifier;
};

Node *Parser::parse_if(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(Precedence::LOWEST, locals);
    next_expression();
    auto true_expr = parse_if_body(locals);
    Node *false_expr;
    if (current_token().is_elsif_keyword()) {
        false_expr = parse_if(locals);
        return new IfNode { current_token(), condition, true_expr, false_expr };
    } else {
        if (current_token().is_else_keyword()) {
            advance();
            false_expr = parse_if_body(locals);
        } else {
            false_expr = new NilNode { current_token() };
        }
        expect(Token::Type::EndKeyword, "if end");
        advance();
        return new IfNode { token, condition, true_expr, false_expr };
    }
}

Node *Parser::parse_if_body(LocalsHashmap &locals) {
    auto body = new BlockNode { current_token() };
    current_token().validate();
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_elsif_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        current_token().validate();
        next_expression();
    }
    if (!current_token().is_elsif_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword())
        throw_unexpected("if end");
    return body->has_one_node() ? body->nodes()[0] : body;
}

void Parser::parse_interpolated_body(LocalsHashmap &locals, InterpolatedNode *node, Token::Type end_token) {
    while (current_token().is_valid() && current_token().type() != end_token) {
        switch (current_token().type()) {
        case Token::Type::EvaluateToStringBegin: {
            advance();
            auto block = new BlockNode { current_token() };
            while (current_token().type() != Token::Type::EvaluateToStringEnd) {
                block->add_node(parse_expression(Precedence::LOWEST, locals));
                skip_newlines();
            }
            advance();
            if (block->has_one_node())
                if (block->nodes()[0]->type() == Node::Type::String)
                    node->add_node(block->nodes()[0]);
                else
                    node->add_node(new EvaluateToStringNode { current_token(), block->nodes()[0] });
            else
                node->add_node(new EvaluateToStringNode { current_token(), block });
            break;
        }
        case Token::Type::String:
            node->add_node(new StringNode { current_token(), current_token().literal_string() });
            advance();
            break;
        default:
            TM_UNREACHABLE();
        }
    }
    if (current_token().type() != end_token)
        TM_UNREACHABLE() // this shouldn't happen -- if it does, there is a bug in the Lexer
};

Node *Parser::parse_interpolated_regexp(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().type() == Token::Type::InterpolatedRegexpEnd) {
        auto regexp_node = new RegexpNode { token, new String };
        auto options = current_token().options();
        if (options)
            regexp_node->set_options(options.value());
        advance();
        return regexp_node;
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedRegexpEnd) {
        auto regexp_node = new RegexpNode { token, current_token().literal_string() };
        advance();
        auto options = current_token().options();
        if (options)
            regexp_node->set_options(options.value());
        advance();
        return regexp_node;
    } else {
        auto interpolated_regexp = new InterpolatedRegexpNode { token };
        parse_interpolated_body(locals, interpolated_regexp, Token::Type::InterpolatedRegexpEnd);
        auto options = current_token().options();
        if (options) {
            int options_int = parse_regexp_options(options.value().ref());
            interpolated_regexp->set_options(options_int);
        }
        advance();
        return interpolated_regexp;
    }
};

int Parser::parse_regexp_options(String &options_string) {
    int options = 0;
    for (char *c = const_cast<char *>(options_string.c_str()); *c != '\0'; ++c) {
        switch (*c) {
        case 'i': // ignore case
            options |= 1;
            break;
        case 'x': // extended
            options |= 2;
            break;
        case 'm': // multiline
            options |= 4;
            break;
        default:
            break;
        }
    }
    return options;
}

Node *Parser::parse_interpolated_shell(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().type() == Token::Type::InterpolatedShellEnd) {
        auto shell = new ShellNode { token, new String("") };
        advance();
        return shell;
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedShellEnd) {
        auto shell = new ShellNode { token, current_token().literal_string() };
        advance();
        advance();
        return shell;
    } else {
        auto interpolated_shell = new InterpolatedShellNode { token };
        parse_interpolated_body(locals, interpolated_shell, Token::Type::InterpolatedShellEnd);
        advance();
        return interpolated_shell;
    }
};

Node *Parser::parse_interpolated_string(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().type() == Token::Type::InterpolatedStringEnd) {
        auto string = new StringNode { token, new String };
        advance();
        return string;
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedStringEnd) {
        auto string = new StringNode { token, current_token().literal_string() };
        advance();
        advance();
        return string;
    } else {
        auto interpolated_string = new InterpolatedStringNode { token };
        parse_interpolated_body(locals, interpolated_string, Token::Type::InterpolatedStringEnd);
        advance();
        return interpolated_string;
    }
};

Node *Parser::parse_lit(LocalsHashmap &locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::Integer:
        advance();
        return new IntegerNode { token, token.get_integer() };
    case Token::Type::Float:
        advance();
        return new FloatNode { token, token.get_double() };
    default:
        TM_UNREACHABLE();
    }
};

Node *Parser::parse_keyword_args(LocalsHashmap &locals, bool bare) {
    auto precedence = bare ? Precedence::BARECALLARGS : Precedence::CALLARGS;
    auto hash = new HashNode { current_token() };
    if (current_token().type() == Token::Type::SymbolKey) {
        hash->add_node(parse_symbol(locals));
    } else {
        hash->add_node(parse_expression(precedence, locals));
        expect(Token::Type::HashRocket, "hash rocket");
        advance();
    }
    hash->add_node(parse_expression(precedence, locals));
    while (current_token().type() == Token::Type::Comma) {
        advance();
        if (current_token().type() == Token::Type::SymbolKey) {
            hash->add_node(parse_symbol(locals));
        } else {
            hash->add_node(parse_expression(precedence, locals));
            expect(Token::Type::HashRocket, "hash rocket");
            advance();
        }
        hash->add_node(parse_expression(precedence, locals));
    }
    return hash;
}

Node *Parser::parse_keyword_splat(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new KeywordSplatNode { token, parse_expression(Precedence::SPLAT, locals) };
}

SharedPtr<String> Parser::parse_method_name(LocalsHashmap &) {
    SharedPtr<String> name = new String("");
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName:
        name = current_token().literal_string();
        break;
    default:
        if (token.is_operator())
            name = new String(current_token().type_value());
        else
            throw_unexpected("method name");
    }
    advance();
    return name;
}

Node *Parser::parse_module(LocalsHashmap &) {
    auto token = current_token();
    advance();
    LocalsHashmap our_locals { TM::HashType::String };
    auto name = parse_class_or_module_name(our_locals);
    auto body = parse_body(our_locals, Precedence::LOWEST);
    expect(Token::Type::EndKeyword, "module end");
    advance();
    return new ModuleNode { token, name, body };
}

Node *Parser::parse_next(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
            return new NextNode { token, new NilSexpNode { token } };
        } else {
            auto arg = parse_expression(Precedence::BARECALLARGS, locals);
            expect(Token::Type::RParen, "break closing paren");
            advance();
            return new NextNode { token, arg };
        }
    } else if (!current_token().is_end_of_expression()) {
        auto value = parse_expression(Precedence::BARECALLARGS, locals);
        if (current_token().is_comma()) {
            auto array = new ArrayNode { token };
            array->add_node(value);
            while (current_token().is_comma()) {
                advance();
                array->add_node(parse_expression(Precedence::BARECALLARGS, locals));
            }
            value = array;
        }
        return new NextNode { token, value };
    }
    return new NextNode { token };
}

Node *Parser::parse_nil(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new NilSexpNode { token };
}

Node *Parser::parse_not(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto precedence = get_precedence(token);
    auto node = new CallNode {
        token,
        parse_expression(precedence, locals),
        new String("!")
    };
    return node;
}

Node *Parser::parse_regexp(LocalsHashmap &locals) {
    auto token = current_token();
    auto regexp = new RegexpNode { token, token.literal_string() };
    regexp->set_options(token.options().value());
    advance();
    return regexp;
};

Node *Parser::parse_return(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    Node *value;
    if (current_token().is_end_of_expression()) {
        value = new NilNode { token };
    } else {
        value = parse_expression(Precedence::BARECALLARGS, locals);
    }
    if (current_token().is_comma()) {
        auto array = new ArrayNode { current_token() };
        array->add_node(value);
        while (current_token().is_comma()) {
            advance();
            array->add_node(parse_expression(Precedence::BARECALLARGS, locals));
        }
        value = array;
    }
    return new ReturnNode { token, value };
};

Node *Parser::parse_sclass(LocalsHashmap &locals) {
    auto token = current_token();
    advance(); // class
    advance(); // <<
    auto klass = parse_expression(Precedence::BARECALLARGS, locals);
    auto body = parse_body(locals, Precedence::LOWEST);
    expect(Token::Type::EndKeyword, "sclass end");
    advance();
    return new SclassNode { token, klass, body };
}

Node *Parser::parse_self(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new SelfNode { token };
};

Node *Parser::parse_splat(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new SplatNode { token, parse_expression(Precedence::SPLAT, locals) };
};

Node *Parser::parse_stabby_proc(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    if (current_token().is_lparen()) {
        advance();
        args = parse_def_args(locals);
        expect(Token::Type::RParen, "proc args closing paren");
        advance();
    } else if (current_token().is_bare_name()) {
        args = parse_def_args(locals);
    }
    if (current_token().type() != Token::Type::DoKeyword && current_token().type() != Token::Type::LCurlyBrace)
        throw_unexpected("block");
    return new StabbyProcNode { token, *args };
};

Node *Parser::parse_string(LocalsHashmap &locals) {
    auto token = current_token();
    auto string = new StringNode { token, token.literal_string() };
    advance();
    return string;
};

Node *Parser::parse_super(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto node = new SuperNode { token };
    if (current_token().is_lparen()) {
        node->set_parens(true);
        advance();
        if (current_token().is_rparen()) {
            advance();
        } else {
            parse_call_args(node, locals, false);
            expect(Token::Type::RParen, "super closing paren");
            advance();
        }
    } else if (!current_token().is_end_of_expression()) {
        parse_call_args(node, locals, true);
    }
    return node;
};

Node *Parser::parse_symbol(LocalsHashmap &locals) {
    auto token = current_token();
    auto symbol = new SymbolNode { token, current_token().literal_string() };
    advance();
    return symbol;
};

Node *Parser::parse_top_level_constant(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<String> name = new String("");
    auto name_token = current_token();
    auto identifier = static_cast<IdentifierNode *>(parse_identifier(locals));
    switch (identifier->token_type()) {
    case Token::Type::BareName:
    case Token::Type::Constant:
        name = identifier->name();
        break;
    default:
        throw_unexpected(name_token, ":: identifier name");
    }
    return new Colon3Node { token, name };
}

Node *Parser::parse_unary_operator(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto precedence = get_precedence(token);
    auto receiver = parse_expression(precedence, locals);
    switch (receiver->type()) {
    case Node::Type::Integer: {
        auto number = static_cast<IntegerNode *>(receiver)->number();
        if (token.type() == Token::Type::Minus)
            number = -number;
        return new IntegerNode {
            Token { Token::Type::Integer, number, token.file(), token.line(), token.column() },
            number,
        };
    }
    case Node::Type::Float: {
        auto number = static_cast<FloatNode *>(receiver)->number();
        if (token.type() == Token::Type::Minus)
            number = -number;
        return new FloatNode {
            Token { Token::Type::Float, number, token.file(), token.line(), token.column() },
            number,
        };
    }
    default: {
        SharedPtr<String> message = new String("");
        switch (token.type()) {
        case Token::Type::Minus:
            *message = "-@";
            break;
        case Token::Type::Plus:
            *message = "+@";
            break;
        default:
            TM_UNREACHABLE();
        }
        return new CallNode { token, receiver, message };
    }
    }
}

Node *Parser::parse_word_array(LocalsHashmap &locals) {
    auto token = current_token();
    auto array = new ArrayNode { token };
    auto literal = token.literal();
    size_t len = strlen(literal);
    if (len > 0) {
        String string;
        for (size_t i = 0; i < len; i++) {
            auto c = literal[i];
            switch (c) {
            case ' ':
                array->add_node(new StringNode { token, new String(string) });
                string = "";
                break;
            default:
                string.append_char(c);
            }
        }
        array->add_node(new StringNode { token, new String(string) });
    }
    advance();
    return array;
}

Node *Parser::parse_word_symbol_array(LocalsHashmap &locals) {
    auto token = current_token();
    auto array = new ArrayNode { token };
    auto literal = token.literal();
    size_t len = strlen(literal);
    if (len > 0) {
        String string;
        for (size_t i = 0; i < len; i++) {
            auto c = literal[i];
            switch (c) {
            case ' ':
                array->add_node(new SymbolNode { token, new String(string) });
                string = "";
                break;
            default:
                string.append_char(c);
            }
        }
        array->add_node(new SymbolNode { token, new String(string) });
    }
    advance();
    return array;
}

Node *Parser::parse_yield(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto node = new YieldNode { token };
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
        } else {
            parse_call_args(node, locals, false);
            expect(Token::Type::RParen, "yield closing paren");
            advance();
        }
    } else if (!current_token().is_end_of_expression()) {
        parse_call_args(node, locals, true);
    }
    return node;
};

Node *Parser::parse_assignment_expression(Node *left, LocalsHashmap &locals) {
    return parse_assignment_expression(left, locals, true);
}

Node *Parser::parse_assignment_expression(Node *left, LocalsHashmap &locals, bool allow_multiple) {
    auto token = current_token();
    if (left->type() == Node::Type::Splat) {
        left = parse_multiple_assignment_expression(left, locals);
    }
    switch (left->type()) {
    case Node::Type::Identifier: {
        auto left_identifier = static_cast<IdentifierNode *>(left);
        left_identifier->add_to_locals(locals);
        advance();
        auto value = parse_assignment_expression_value(false, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    case Node::Type::Call:
    case Node::Type::Colon2:
    case Node::Type::Colon3: {
        advance();
        auto value = parse_assignment_expression_value(false, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    case Node::Type::MultipleAssignment: {
        static_cast<MultipleAssignmentNode *>(left)->add_locals(locals);
        advance();
        auto value = parse_assignment_expression_value(true, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    default:
        throw_unexpected(left->token(), "left side of assignment");
    }
};

Node *Parser::parse_assignment_expression_value(bool to_array, LocalsHashmap &locals, bool allow_multiple) {
    auto token = current_token();
    auto value = parse_expression(Precedence::ASSIGNMENT, locals);
    bool is_splat;

    if (allow_multiple && current_token().type() == Token::Type::Comma) {
        auto array = new ArrayNode { token };
        array->add_node(value);
        while (current_token().type() == Token::Type::Comma) {
            advance();
            array->add_node(parse_expression(Precedence::ASSIGNMENT, locals));
        }
        value = array;
        is_splat = true;
    } else if (value->type() == Node::Type::Splat) {
        is_splat = true;
    } else {
        is_splat = false;
    }

    if (is_splat) {
        if (to_array) {
            return value;
        } else {
            return new SplatValueNode { token, value };
        }
    } else {
        if (to_array) {
            return new ToArrayNode { token, value };
        } else {
            return value;
        }
    }
}

Node *Parser::parse_iter_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    LocalsHashmap our_locals { locals }; // copy!
    bool curly_brace = current_token().type() == Token::Type::LCurlyBrace;
    advance();
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    switch (left->type()) {
    case Node::Type::Identifier:
    case Node::Type::Call:
        if (current_token().is_block_arg_delimiter()) {
            advance();
            args = parse_iter_args(our_locals);
            expect(Token::Type::BitwiseOr, "end of block args");
            advance();
        }
        break;
    case Node::Type::StabbyProc:
        for (auto arg : static_cast<StabbyProcNode *>(left)->args()) {
            args->push(arg->clone());
        }
        break;
    default:
        throw_unexpected(left->token(), "call for left side of iter");
    }
    auto end_token_type = curly_brace ? Token::Type::RCurlyBrace : Token::Type::EndKeyword;
    auto body = parse_body(our_locals, Precedence::LOWEST, end_token_type);
    expect(end_token_type, "block end");
    advance();
    return new IterNode { token, left, *args, body };
}

SharedPtr<Vector<Node *>> Parser::parse_iter_args(LocalsHashmap &locals) {
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    args->push(parse_def_single_arg(locals));
    while (current_token().is_comma()) {
        advance();
        if (current_token().is_block_arg_delimiter()) {
            // trailing comma with no additional arg
            break;
        }
        args->push(parse_def_single_arg(locals));
    }
    return args;
}

Node *Parser::parse_call_expression_with_parens(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    CallNode *call_node;
    switch (left->type()) {
    case Node::Type::Identifier:
        call_node = new CallNode {
            token,
            new NilNode { token },
            static_cast<IdentifierNode *>(left)->name(),
        };
        break;
    case Node::Type::Call:
        call_node = static_cast<CallNode *>(left);
        break;
    case Node::Type::SafeCall:
        call_node = static_cast<SafeCallNode *>(left);
        break;
    default:
        TM_UNREACHABLE();
    }
    advance();
    if (!current_token().is_rparen())
        parse_call_args(call_node, locals, false);
    expect(Token::Type::RParen, "call rparen");
    advance();
    return call_node;
}

Node *Parser::parse_call_arg(LocalsHashmap &locals, bool bare, bool *keyword_args) {
    if ((current_token().type() == Token::Type::Symbol && peek_token().type() == Token::Type::HashRocket) || current_token().type() == Token::Type::SymbolKey) {
        auto hash = parse_keyword_args(locals, bare);
        *keyword_args = true;
        return hash;
    } else {
        auto arg = parse_expression(bare ? Precedence::BARECALLARGS : Precedence::CALLARGS, locals);
        *keyword_args = false;
        if (current_token().type() == Token::Type::Equal) {
            return parse_assignment_expression(arg, locals, false);
        } else {
            return arg;
        }
    }
}

void Parser::parse_call_args(NodeWithArgs *node, LocalsHashmap &locals, bool bare) {
    bool keyword_args;
    node->add_arg(parse_call_arg(locals, bare, &keyword_args));
    if (keyword_args) {
        return;
    }
    while (current_token().is_comma()) {
        advance();
        auto token = current_token();
        if (token.is_rparen()) {
            // trailing comma with no additional arg
            break;
        }
        node->add_arg(parse_call_arg(locals, bare, &keyword_args));
        if (keyword_args) {
            break;
        }
    }
}

Node *Parser::parse_call_expression_without_parens(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    CallNode *call_node;
    switch (left->type()) {
    case Node::Type::Identifier:
        call_node = new CallNode {
            token,
            new NilNode { token },
            static_cast<IdentifierNode *>(left)->name(),
        };
        break;
    case Node::Type::Call:
        call_node = static_cast<CallNode *>(left);
        break;
    case Node::Type::SafeCall:
        call_node = static_cast<SafeCallNode *>(left);
        break;
    default:
        TM_UNREACHABLE();
    }
    switch (token.type()) {
    case Token::Type::Comma:
    case Token::Type::Eof:
    case Token::Type::Eol:
    case Token::Type::RBracket:
    case Token::Type::RCurlyBrace:
    case Token::Type::RParen:
        break;
    default:
        parse_call_args(call_node, locals, true);
    }
    return call_node;
}

Node *Parser::parse_constant_resolution_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto name_token = current_token();
    auto identifier = static_cast<IdentifierNode *>(parse_identifier(locals));
    switch (identifier->token_type()) {
    case Token::Type::BareName: {
        auto name = identifier->name();
        return new CallNode { token, left, name };
    }
    case Token::Type::Constant: {
        auto name = identifier->name();
        return new Colon2Node { token, left, name };
    }
    default:
        throw_unexpected(name_token, ":: identifier name");
    }
}

Node *Parser::parse_infix_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    auto op = current_token();
    auto precedence = get_precedence(token, left);
    advance();
    auto right = parse_expression(precedence, locals);
    auto *node = new CallNode {
        token,
        left,
        new String(op.type_value()),
    };
    node->add_arg(right);
    return node;
};

Node *Parser::parse_logical_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::And: {
        advance();
        auto right = parse_expression(Precedence::LOGICALAND, locals);
        if (left->type() == Node::Type::LogicalAnd) {
            return regroup<LogicalAndNode>(token, left, right);
        } else {
            return new LogicalAndNode { token, left, right };
        }
    }
    case Token::Type::AndKeyword: {
        advance();
        auto right = parse_expression(Precedence::COMPOSITION, locals);
        if (left->type() == Node::Type::LogicalAnd) {
            return regroup<LogicalAndNode>(token, left, right);
        } else {
            return new LogicalAndNode { token, left, right };
        }
    }
    case Token::Type::Or: {
        advance();
        auto right = parse_expression(Precedence::LOGICALOR, locals);
        if (left->type() == Node::Type::LogicalOr) {
            return regroup<LogicalOrNode>(token, left, right);
        } else {
            return new LogicalOrNode { token, left, right };
        }
    }
    case Token::Type::OrKeyword: {
        advance();
        auto right = parse_expression(Precedence::COMPOSITION, locals);
        if (left->type() == Node::Type::LogicalOr) {
            return regroup<LogicalOrNode>(token, left, right);
        } else {
            return new LogicalOrNode { token, left, right };
        }
    }
    default:
        TM_UNREACHABLE();
    }
}

Node *Parser::parse_match_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto arg = parse_expression(Precedence::EQUALITY, locals);
    if (left->type() == Node::Type::Regexp) {
        return new MatchNode { token, static_cast<RegexpNode *>(left), arg, true };
    } else if (arg->type() == Node::Type::Regexp) {
        return new MatchNode { token, static_cast<RegexpNode *>(arg), left, false };
    } else {
        auto *node = new CallNode {
            token,
            left,
            new String("=~"),
        };
        node->add_arg(arg);
        return node;
    }
}

Node *Parser::parse_not_match_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    auto match = parse_match_expression(left, locals);
    return new NotNode { token, match };
}

Node *Parser::parse_op_assign_expression(Node *left, LocalsHashmap &locals) {
    if (left->type() == Node::Type::Call)
        return parse_op_attr_assign_expression(left, locals);
    if (left->type() != Node::Type::Identifier)
        throw_unexpected(left->token(), "identifier");
    auto left_identifier = static_cast<IdentifierNode *>(left);
    left_identifier->set_is_lvar(true);
    left_identifier->add_to_locals(locals);
    auto token = current_token();
    advance();
    switch (token.type()) {
    case Token::Type::AndEqual:
        return new OpAssignAndNode { token, left_identifier, parse_expression(Precedence::ASSIGNMENT, locals) };
    case Token::Type::BitwiseAndEqual:
    case Token::Type::BitwiseOrEqual:
    case Token::Type::BitwiseXorEqual:
    case Token::Type::DivideEqual:
    case Token::Type::ExponentEqual:
    case Token::Type::LeftShiftEqual:
    case Token::Type::MinusEqual:
    case Token::Type::ModulusEqual:
    case Token::Type::MultiplyEqual:
    case Token::Type::PlusEqual:
    case Token::Type::RightShiftEqual: {
        auto op = new String(token.type_value());
        op->chomp();
        return new OpAssignNode { token, op, left_identifier, parse_expression(Precedence::ASSIGNMENT, locals) };
    }
    case Token::Type::OrEqual:
        return new OpAssignOrNode { token, left_identifier, parse_expression(Precedence::ASSIGNMENT, locals) };
    default:
        TM_UNREACHABLE();
    }
}

Node *Parser::parse_op_attr_assign_expression(Node *left, LocalsHashmap &locals) {
    if (left->type() != Node::Type::Call)
        throw_unexpected(left->token(), "call");
    auto left_call = static_cast<CallNode *>(left);
    auto token = current_token();
    advance();
    auto op = new String(token.type_value());
    op->chomp();
    SharedPtr<String> message = new String(left_call->message().ref());
    message->append_char('=');
    return new OpAssignAccessorNode {
        token,
        op,
        left_call->receiver(),
        message,
        parse_expression(Precedence::OPASSIGNMENT, locals),
        left_call->args(),
    };
}

Node *Parser::parse_proc_call_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    advance();
    auto call_node = new CallNode {
        token,
        left,
        new String("call"),
    };
    if (!current_token().is_rparen())
        parse_call_args(call_node, locals, false);
    expect(Token::Type::RParen, "proc call right paren");
    advance();
    return call_node;
}

Node *Parser::parse_range_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    Node *right;
    try {
        right = parse_expression(Precedence::RANGE, locals);
    } catch (SyntaxError &e) {
        // NOTE: I'm not sure if this is the "right" way to handle an endless range,
        // but it seems to be effective for the tests I threw at it. ¯\_(ツ)_/¯
        right = new NilNode { token };
    }
    return new RangeNode { token, left, right, token.type() == Token::Type::DotDotDot };
}

Node *Parser::parse_ref_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto call_node = new CallNode {
        token,
        left,
        new String("[]"),
    };
    if (token.type() == Token::Type::LBracketRBracket)
        return call_node;
    if (current_token().type() != Token::Type::RBracket)
        parse_call_args(call_node, locals, false);
    expect(Token::Type::RBracket, "element reference right bracket");
    advance();
    return call_node;
}

Node *Parser::parse_safe_send_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    expect(Token::Type::BareName, "safe navigator method name");
    auto name = static_cast<IdentifierNode *>(parse_identifier(locals));
    auto call_node = new SafeCallNode {
        token,
        left,
        name->name(),
    };
    return call_node;
}

Node *Parser::parse_send_expression(Node *left, LocalsHashmap &locals) {
    auto dot_token = current_token();
    advance();
    auto name_token = current_token();
    SharedPtr<String> name = new String("");
    switch (name_token.type()) {
    case Token::Type::BareName:
        name = static_cast<IdentifierNode *>(parse_identifier(locals))->name();
        break;
    case Token::Type::ClassKeyword:
    case Token::Type::BeginKeyword:
    case Token::Type::EndKeyword:
        *name = name_token.type_value();
        advance();
        break;
    default:
        if (name_token.is_operator()) {
            *name = name_token.type_value();
            advance();
        } else {
            throw_unexpected("send method name");
        }
    };
    return new CallNode {
        dot_token,
        left,
        name,
    };
}

Node *Parser::parse_ternary_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    expect(Token::Type::TernaryQuestion, "ternary question");
    advance();
    auto true_expr = parse_expression(Precedence::TERNARY, locals);
    expect(Token::Type::TernaryColon, "ternary colon");
    advance();
    auto false_expr = parse_expression(Precedence::TERNARY, locals);
    return new IfNode { token, left, true_expr, false_expr };
}

Node *Parser::parse_unless(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(Precedence::LOWEST, locals);
    next_expression();
    auto false_expr = parse_if_body(locals);
    Node *true_expr;
    if (current_token().is_else_keyword()) {
        advance();
        true_expr = parse_if_body(locals);
    } else {
        true_expr = new NilNode { current_token() };
    }
    expect(Token::Type::EndKeyword, "unless end");
    advance();
    return new IfNode { token, condition, true_expr, false_expr };
}

Node *Parser::parse_while(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(Precedence::LOWEST, locals);
    next_expression();
    auto body = parse_body(locals, Precedence::LOWEST);
    expect(Token::Type::EndKeyword, "while end");
    advance();
    switch (token.type()) {
    case Token::Type::UntilKeyword:
        return new UntilNode { token, condition, body, true };
    case Token::Type::WhileKeyword:
        return new WhileNode { token, condition, body, true };
    default:
        TM_UNREACHABLE();
    }
}

Parser::parse_null_fn Parser::null_denotation(Token::Type type, Precedence precedence) {
    using Type = Token::Type;
    switch (type) {
    case Type::AliasKeyword:
        return &Parser::parse_alias;
    case Type::LBracket:
    case Type::LBracketRBracket:
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
    case Type::DefinedKeyword:
        return &Parser::parse_defined;
    case Type::DotDot:
    case Type::DotDotDot:
        return &Parser::parse_beginless_range;
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
        return &Parser::parse_identifier;
    case Type::IfKeyword:
        return &Parser::parse_if;
    case Type::InterpolatedRegexpBegin:
        return &Parser::parse_interpolated_regexp;
    case Type::InterpolatedShellBegin:
        return &Parser::parse_interpolated_shell;
    case Type::InterpolatedStringBegin:
        return &Parser::parse_interpolated_string;
    case Type::Integer:
    case Type::Float:
        return &Parser::parse_lit;
    case Type::Minus:
    case Type::Plus:
        return &Parser::parse_unary_operator;
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
    case Type::Exponent:
        return &Parser::parse_keyword_splat;
    case Type::Arrow:
        return &Parser::parse_stabby_proc;
    case Type::String:
        return &Parser::parse_string;
    case Type::SuperKeyword:
        return &Parser::parse_super;
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
}

Parser::parse_left_fn Parser::left_denotation(Token &token, Node *left) {
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
    case Type::Comparison:
    case Type::Divide:
    case Type::EqualEqual:
    case Type::EqualEqualEqual:
    case Type::Exponent:
    case Type::GreaterThan:
    case Type::GreaterThanOrEqual:
    case Type::LeftShift:
    case Type::LessThan:
    case Type::LessThanOrEqual:
    case Type::Minus:
    case Type::Modulus:
    case Type::Multiply:
    case Type::NotEqual:
    case Type::Plus:
    case Type::RightShift:
        return &Parser::parse_infix_expression;
    case Type::Comma:
        return &Parser::parse_multiple_assignment_expression;
    case Type::DoKeyword:
    case Type::LCurlyBrace:
        return &Parser::parse_iter_expression;
    case Type::And:
    case Type::AndKeyword:
    case Type::Or:
    case Type::OrKeyword:
        return &Parser::parse_logical_expression;
    case Type::Match:
        return &Parser::parse_match_expression;
    case Type::IfKeyword:
    case Type::UnlessKeyword:
    case Type::WhileKeyword:
    case Type::UntilKeyword:
        return &Parser::parse_modifier_expression;
    case Type::NotMatch:
        return &Parser::parse_not_match_expression;
    case Type::AndEqual:
    case Type::BitwiseAndEqual:
    case Type::BitwiseOrEqual:
    case Type::BitwiseXorEqual:
    case Type::DivideEqual:
    case Type::ExponentEqual:
    case Type::LeftShiftEqual:
    case Type::MinusEqual:
    case Type::ModulusEqual:
    case Type::MultiplyEqual:
    case Type::OrEqual:
    case Type::PlusEqual:
    case Type::RightShiftEqual:
        return &Parser::parse_op_assign_expression;
    case Type::DotDot:
    case Type::DotDotDot:
        return &Parser::parse_range_expression;
    case Type::LBracket:
    case Type::LBracketRBracket: {
        if (treat_left_bracket_as_element_reference(left, token))
            return &Parser::parse_ref_expression;
        break;
    }
    case Type::SafeNavigation:
        return &Parser::parse_safe_send_expression;
    case Type::Dot:
        if (peek_token().is_lparen()) {
            return &Parser::parse_proc_call_expression;
        } else {
            return &Parser::parse_send_expression;
        }
    case Type::TernaryQuestion:
        return &Parser::parse_ternary_expression;
    default:
        break;
    }
    if (is_first_arg_of_call_without_parens(left, token))
        return &Parser::parse_call_expression_without_parens;
    return nullptr;
}

bool Parser::is_first_arg_of_call_without_parens(Node *left, Token &token) {
    return left->is_callable() && token.can_be_first_arg_of_implicit_call();
}

Token Parser::current_token() const {
    if (m_index < m_tokens->size()) {
        return m_tokens->at(m_index);
    } else {
        return Token::invalid();
    }
}

Token Parser::peek_token() const {
    if (m_index + 1 < m_tokens->size()) {
        return (*m_tokens)[m_index + 1];
    } else {
        return Token::invalid();
    }
}

void Parser::next_expression() {
    auto token = current_token();
    if (!token.is_end_of_expression())
        throw_unexpected("end-of-line");
    skip_newlines();
}

void Parser::skip_newlines() {
    while (current_token().is_eol())
        advance();
}

void Parser::expect(Token::Type type, const char *expected) {
    if (current_token().type() != type)
        throw_unexpected(expected);
}

void Parser::throw_unexpected(const Token &token, const char *expected) {
    auto file = token.file() ? String(*token.file()) : String("(unknown)");
    auto line = token.line() + 1;
    auto type = token.type_value();
    auto literal = token.literal();
    if (token.type() == Token::Type::Invalid)
        throw SyntaxError { String::format("{}#{}: syntax error, unexpected '{}' (expected: '{}')", file, line, token.literal(), expected) };
    else if (!type)
        throw SyntaxError { String::format("{}#{}: syntax error, expected '{}' (token type: {})", file, line, expected, (long long)token.type()) };
    else if (strcmp(type, "EOF") == 0)
        throw SyntaxError { String::format("{}#{}: syntax error, unexpected end-of-input (expected: '{}')", file, line, expected) };
    else if (literal)
        throw SyntaxError { String::format("{}#{}: syntax error, unexpected {} '{}' (expected: '{}')", file, line, type, literal, expected) };
    else
        throw SyntaxError { String::format("{}#{}: syntax error, unexpected '{}' (expected: '{}')", file, line, type, expected) };
}

void Parser::throw_unexpected(const char *expected) {
    throw_unexpected(current_token(), expected);
}
}
