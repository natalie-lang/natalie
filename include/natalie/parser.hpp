#pragma once

#include <string>

#include "natalie/gc.hpp"
#include "natalie/lexer.hpp"
#include "natalie/node.hpp"
#include "natalie/token.hpp"

namespace Natalie {

struct Parser : public gc {
    Parser(const char *code, const char *file)
        : m_code { code }
        , m_file { file } {
        assert(m_code);
        m_tokens = Lexer { m_code, m_file }.tokens();
    }

    using LocalsVectorPtr = Vector<SymbolValue *> *;

    enum Precedence {
        LOWEST,
        ARRAY, // []
        HASH, // {}
        EXPRMODIFIER, // if/unless/while/until
        CASE, // case/when/else
        SPLAT, // *args
        CALLARGS, // foo a, b
        COMPOSITION, // and/or
        ASSIGNMENT, // =
        OPASSIGNMENT, // += -= *= **= /= %= |= &= ^= >>= <<= ||= &&=
        RANGE, // ..
        TERNARY, // ? :
        ITER, // do/end {}
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
        CONSTANTRESOLUTION, // ::
        UNARY, // ! ~ + -
        EXPONENT, // **
        DOT, // foo.bar foo&.bar
        CALL, // foo()
        REF, // foo[1] / foo[1] = 2
    };

    Node *tree(Env *);

private:
    bool higher_precedence(Token token, Node *left, Precedence current_precedence) {
        auto next_precedence = get_precedence(token, left);
        // trick to make chained assignment right-to-left
        if (current_precedence == ASSIGNMENT && next_precedence == ASSIGNMENT)
            return true;
        return next_precedence > current_precedence;
    }

    Precedence get_precedence(Token token, Node *left = nullptr) {
        switch (token.type()) {
        case Token::Type::Plus:
        case Token::Type::Minus:
            return SUM;
        case Token::Type::Integer:
        case Token::Type::Float:
            if (current_token().has_sign())
                return SUM;
            break;
        case Token::Type::Equal:
            return ASSIGNMENT;
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
            return OPASSIGNMENT;
        case Token::Type::BitwiseAnd:
            return BITWISEAND;
        case Token::Type::BitwiseOr:
        case Token::Type::BitwiseXor:
            return BITWISEOR;
        case Token::Type::LeftShift:
        case Token::Type::RightShift:
            return BITWISESHIFT;
        case Token::Type::LParen:
            return CALL;
        case Token::Type::AndKeyword:
        case Token::Type::OrKeyword:
            return COMPOSITION;
        case Token::Type::ConstantResolution:
            return CONSTANTRESOLUTION;
        case Token::Type::Dot:
        case Token::Type::SafeNavigation:
            return DOT;
        case Token::Type::EqualEqual:
        case Token::Type::EqualEqualEqual:
        case Token::Type::NotEqual:
        case Token::Type::Match:
        case Token::Type::NotMatch:
            return EQUALITY;
        case Token::Type::Exponent:
            return EXPONENT;
        case Token::Type::IfKeyword:
        case Token::Type::UnlessKeyword:
        case Token::Type::WhileKeyword:
        case Token::Type::UntilKeyword:
            return EXPRMODIFIER;
        case Token::Type::DoKeyword:
        case Token::Type::LCurlyBrace:
            return ITER;
        case Token::Type::LessThan:
        case Token::Type::LessThanOrEqual:
        case Token::Type::GreaterThan:
        case Token::Type::GreaterThanOrEqual:
            return LESSGREATER;
        case Token::Type::And:
            return LOGICALAND;
        case Token::Type::NotKeyword:
            return LOGICALNOT;
        case Token::Type::Or:
            return LOGICALOR;
        case Token::Type::Divide:
        case Token::Type::Modulus:
        case Token::Type::Multiply:
            return PRODUCT;
        case Token::Type::DotDot:
        case Token::Type::DotDotDot:
            return RANGE;
        case Token::Type::LBracket:
            if (left && treat_left_bracket_as_element_reference(left, current_token()))
                return REF;
            break;
        case Token::Type::TernaryQuestion:
        case Token::Type::TernaryColon:
            return TERNARY;
        case Token::Type::Not:
            return UNARY;
        default:
            break;
        }
        if (left && is_first_arg_of_call_without_parens(current_token(), left))
            return CALL;
        return LOWEST;
    }

    bool is_first_arg_of_call_without_parens(Token, Node *);

    Node *parse_expression(Env *, Precedence, LocalsVectorPtr);

    BlockNode *parse_body(Env *, LocalsVectorPtr, Precedence, Token::Type = Token::Type::EndKeyword);
    BlockNode *parse_body(Env *, LocalsVectorPtr, Precedence, Vector<Token::Type> *, const char *);
    BlockNode *parse_case_when_body(Env *, LocalsVectorPtr);
    Node *parse_if_body(Env *, LocalsVectorPtr);

    Node *parse_alias(Env *, LocalsVectorPtr);
    SymbolNode *parse_alias_arg(Env *, LocalsVectorPtr, const char *);
    Node *parse_array(Env *, LocalsVectorPtr);
    Node *parse_begin(Env *, LocalsVectorPtr);
    Node *parse_block_pass(Env *, LocalsVectorPtr);
    Node *parse_bool(Env *, LocalsVectorPtr);
    Node *parse_break(Env *, LocalsVectorPtr);
    Node *parse_class(Env *, LocalsVectorPtr);
    Node *parse_case(Env *, LocalsVectorPtr);
    Node *parse_comma_separated_identifiers(Env *, LocalsVectorPtr);
    void parse_comma_separated_expressions(Env *, ArrayNode *, LocalsVectorPtr);
    Node *parse_constant(Env *, LocalsVectorPtr);
    Node *parse_def(Env *, LocalsVectorPtr);
    Node *parse_defined(Env *, LocalsVectorPtr);
    Vector<Node *> *parse_def_args(Env *, LocalsVectorPtr);
    Node *parse_def_single_arg(Env *, LocalsVectorPtr);
    Node *parse_file_constant(Env *, LocalsVectorPtr);
    Node *parse_group(Env *, LocalsVectorPtr);
    Node *parse_hash(Env *, LocalsVectorPtr);
    Node *parse_identifier(Env *, LocalsVectorPtr);
    Node *parse_if(Env *, LocalsVectorPtr);
    void parse_interpolated_body(Env *, LocalsVectorPtr, InterpolatedNode *, Token::Type);
    Node *parse_interpolated_regexp(Env *, LocalsVectorPtr);
    Node *parse_interpolated_shell(Env *, LocalsVectorPtr);
    Node *parse_interpolated_string(Env *, LocalsVectorPtr);
    Node *parse_lit(Env *, LocalsVectorPtr);
    Node *parse_keyword_args(Env *, LocalsVectorPtr);
    Node *parse_module(Env *, LocalsVectorPtr);
    Node *parse_next(Env *, LocalsVectorPtr);
    Node *parse_nil(Env *, LocalsVectorPtr);
    Node *parse_not(Env *, LocalsVectorPtr);
    Node *parse_regexp(Env *, LocalsVectorPtr);
    Node *parse_return(Env *, LocalsVectorPtr);
    Node *parse_self(Env *, LocalsVectorPtr);
    Node *parse_splat(Env *, LocalsVectorPtr);
    Node *parse_stabby_proc(Env *, LocalsVectorPtr);
    Node *parse_string(Env *, LocalsVectorPtr);
    Node *parse_super(Env *, LocalsVectorPtr);
    Node *parse_symbol(Env *, LocalsVectorPtr);
    Node *parse_statement_keyword(Env *, LocalsVectorPtr);
    Node *parse_top_level_constant(Env *, LocalsVectorPtr);
    Node *parse_unless(Env *, LocalsVectorPtr);
    Node *parse_while(Env *, LocalsVectorPtr);
    Node *parse_word_array(Env *, LocalsVectorPtr);
    Node *parse_word_symbol_array(Env *, LocalsVectorPtr);
    Node *parse_yield(Env *, LocalsVectorPtr);

    Node *parse_assignment_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_call_expression_without_parens(Env *, Node *, LocalsVectorPtr);
    Node *parse_call_expression_with_parens(Env *, Node *, LocalsVectorPtr);
    void parse_call_args(Env *, NodeWithArgs *, LocalsVectorPtr);
    Node *parse_constant_resolution_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_infix_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_proc_call_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_iter_expression(Env *, Node *, LocalsVectorPtr);
    Vector<Node *> *parse_iter_args(Env *, LocalsVectorPtr);
    Node *parse_logical_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_modifier_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_not_match_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_op_assign_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_op_attr_assign_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_range_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_ref_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_safe_send_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_send_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_ternary_expression(Env *, Node *, LocalsVectorPtr);

    using parse_null_fn = Node *(Parser::*)(Env *, LocalsVectorPtr);
    using parse_left_fn = Node *(Parser::*)(Env *, Node *, LocalsVectorPtr);

    parse_null_fn null_denotation(Token::Type, Precedence);
    parse_left_fn left_denotation(Token, Node *);

    bool treat_left_bracket_as_element_reference(Node *left, Token token) {
        return !token.whitespace_precedes() || (left->type() == Node::Type::Identifier && static_cast<IdentifierNode *>(left)->is_lvar());
    }

    Token current_token();
    Token peek_token();

    void next_expression(Env *);
    void skip_newlines();

    void expect(Env *, Token::Type, const char *);
    [[noreturn]] void raise_unexpected(Env *, Token, const char *);
    [[noreturn]] void raise_unexpected(Env *, const char *);

    void advance() { m_index++; }

    const char *m_code { nullptr };
    const char *m_file { nullptr };
    size_t m_index { 0 };
    Vector<Token> *m_tokens { nullptr };
};
}
