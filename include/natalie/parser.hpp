#pragma once

#include "natalie/gc.hpp"
#include "natalie/lexer.hpp"
#include "natalie/node.hpp"
#include "natalie/token.hpp"

namespace Natalie {

using namespace TM;

class Parser : public Cell {
public:
    class SyntaxError {
    public:
        NAT_MAKE_NONCOPYABLE(SyntaxError);

        SyntaxError(const char *message)
            : m_message { strdup(message) } {
            assert(m_message);
        }

        SyntaxError(const String &message)
            : SyntaxError { message.c_str() } { }

        ~SyntaxError() {
            free(m_message);
        }

        const char *message() { return m_message; }

    private:
        char *m_message { nullptr };
    };

    Parser(SharedPtr<String> code, SharedPtr<String> file)
        : m_code { code }
        , m_file { file } {
        m_tokens = Lexer { m_code, m_file }.tokens();
    }

    using LocalsHashmap = TM::Hashmap<const char *>;

    enum Precedence {
        LOWEST,
        ARRAY, // []
        HASH, // {}
        EXPRMODIFIER, // if/unless/while/until
        CASE, // case/when/else
        SPLAT, // *args, **kwargs
        ASSIGNMENT, // =
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
        CONSTANTRESOLUTION, // ::
        UNARY, // ! ~ + -
        EXPONENT, // **
        DOT, // foo.bar foo&.bar
        CALL, // foo()
        REF, // foo[1] / foo[1] = 2
    };

    Node *tree();

    virtual void visit_children(Visitor &visitor) override {
        visitor.visit(m_tokens);
    }

private:
    bool higher_precedence(Token *token, Node *left, Precedence current_precedence) {
        auto next_precedence = get_precedence(token, left);
        // trick to make chained assignment right-to-left
        if (current_precedence == ASSIGNMENT && next_precedence == ASSIGNMENT)
            return true;
        return next_precedence > current_precedence;
    }

    Precedence get_precedence(Token *token, Node *left = nullptr) {
        switch (token->type()) {
        case Token::Type::Plus:
        case Token::Type::Minus:
            return SUM;
        case Token::Type::Integer:
        case Token::Type::Float:
            if (current_token()->has_sign())
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
            return ITER_BLOCK;
        case Token::Type::LCurlyBrace:
            return ITER_CURLY;
        case Token::Type::Comparison:
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
        case Token::Type::LBracketRBracket:
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

    bool is_first_arg_of_call_without_parens(Token *, Node *);

    Node *parse_expression(Precedence, LocalsHashmap &);

    BlockNode *parse_body(LocalsHashmap &, Precedence, Token::Type = Token::Type::EndKeyword);
    BlockNode *parse_body(LocalsHashmap &, Precedence, Vector<Token::Type> &, const char *);
    BlockNode *parse_case_when_body(LocalsHashmap &);
    Node *parse_if_body(LocalsHashmap &);
    BlockNode *parse_def_body(LocalsHashmap &);

    Node *parse_alias(LocalsHashmap &);
    SymbolNode *parse_alias_arg(LocalsHashmap &, const char *);
    Node *parse_array(LocalsHashmap &);
    Node *parse_begin(LocalsHashmap &);
    void parse_rest_of_begin(BeginNode *, LocalsHashmap &);
    Node *parse_beginless_range(LocalsHashmap &);
    Node *parse_block_pass(LocalsHashmap &);
    Node *parse_bool(LocalsHashmap &);
    Node *parse_break(LocalsHashmap &);
    Node *parse_class(LocalsHashmap &);
    Node *parse_case(LocalsHashmap &);
    Node *parse_comma_separated_identifiers(LocalsHashmap &);
    void parse_comma_separated_expressions(ArrayNode *, LocalsHashmap &);
    Node *parse_constant(LocalsHashmap &);
    Node *parse_def(LocalsHashmap &);
    Node *parse_defined(LocalsHashmap &);
    SharedPtr<Vector<Node *>> parse_def_args(LocalsHashmap &);
    Node *parse_def_single_arg(LocalsHashmap &);
    Node *parse_file_constant(LocalsHashmap &);
    Node *parse_group(LocalsHashmap &);
    Node *parse_hash(LocalsHashmap &);
    Node *parse_identifier(LocalsHashmap &);
    Node *parse_if(LocalsHashmap &);
    void parse_interpolated_body(LocalsHashmap &, InterpolatedNode *, Token::Type);
    Node *parse_interpolated_regexp(LocalsHashmap &);
    int parse_regexp_options(String &);
    Node *parse_interpolated_shell(LocalsHashmap &);
    Node *parse_interpolated_string(LocalsHashmap &);
    Node *parse_lit(LocalsHashmap &);
    Node *parse_keyword_args(LocalsHashmap &);
    Node *parse_keyword_splat(LocalsHashmap &);
    SharedPtr<String> parse_method_name(LocalsHashmap &);
    Node *parse_module(LocalsHashmap &);
    Node *parse_next(LocalsHashmap &);
    Node *parse_nil(LocalsHashmap &);
    Node *parse_not(LocalsHashmap &);
    Node *parse_regexp(LocalsHashmap &);
    Node *parse_return(LocalsHashmap &);
    Node *parse_sclass(LocalsHashmap &);
    Node *parse_self(LocalsHashmap &);
    Node *parse_splat(LocalsHashmap &);
    Node *parse_stabby_proc(LocalsHashmap &);
    Node *parse_string(LocalsHashmap &);
    Node *parse_super(LocalsHashmap &);
    Node *parse_symbol(LocalsHashmap &);
    Node *parse_statement_keyword(LocalsHashmap &);
    Node *parse_top_level_constant(LocalsHashmap &);
    Node *parse_unary_operator(LocalsHashmap &);
    Node *parse_unless(LocalsHashmap &);
    Node *parse_while(LocalsHashmap &);
    Node *parse_word_array(LocalsHashmap &);
    Node *parse_word_symbol_array(LocalsHashmap &);
    Node *parse_yield(LocalsHashmap &);

    Node *parse_assignment_expression(Node *, LocalsHashmap &);
    Node *parse_call_expression_without_parens(Node *, LocalsHashmap &);
    Node *parse_call_expression_with_parens(Node *, LocalsHashmap &);
    void parse_call_args(NodeWithArgs *, LocalsHashmap &, bool = false);
    Node *parse_constant_resolution_expression(Node *, LocalsHashmap &);
    Node *parse_infix_expression(Node *, LocalsHashmap &);
    Node *parse_proc_call_expression(Node *, LocalsHashmap &);
    Node *parse_iter_expression(Node *, LocalsHashmap &);
    SharedPtr<Vector<Node *>> parse_iter_args(LocalsHashmap &);
    Node *parse_logical_expression(Node *, LocalsHashmap &);
    Node *parse_match_expression(Node *, LocalsHashmap &);
    Node *parse_modifier_expression(Node *, LocalsHashmap &);
    Node *parse_not_match_expression(Node *, LocalsHashmap &);
    Node *parse_op_assign_expression(Node *, LocalsHashmap &);
    Node *parse_op_attr_assign_expression(Node *, LocalsHashmap &);
    Node *parse_range_expression(Node *, LocalsHashmap &);
    Node *parse_ref_expression(Node *, LocalsHashmap &);
    Node *parse_safe_send_expression(Node *, LocalsHashmap &);
    Node *parse_send_expression(Node *, LocalsHashmap &);
    Node *parse_ternary_expression(Node *, LocalsHashmap &);

    using parse_null_fn = Node *(Parser::*)(LocalsHashmap &);
    using parse_left_fn = Node *(Parser::*)(Node *, LocalsHashmap &);

    parse_null_fn null_denotation(Token::Type, Precedence);
    parse_left_fn left_denotation(Token *, Node *);

    bool treat_left_bracket_as_element_reference(Node *left, Token *token) {
        return !token->whitespace_precedes() || (left->type() == Node::Type::Identifier && static_cast<IdentifierNode *>(left)->is_lvar());
    }

    // convert ((x and y) and z) to (x and (y and z))
    template <typename T>
    Node *regroup(Token *token, Node *left, Node *right) {
        auto left_node = static_cast<T *>(left);
        return new T { left_node->token(), left_node->left(), new T { token, left_node->right(), right } };
    };

    Token *current_token();
    Token *peek_token();

    void next_expression();
    void skip_newlines();

    void expect(Token::Type, const char *);
    [[noreturn]] void throw_unexpected(Token *, const char *);
    [[noreturn]] void throw_unexpected(const char *);

    void advance() { m_index++; }
    void rewind() { m_index--; }

    SharedPtr<String> m_code;
    SharedPtr<String> m_file;
    size_t m_index { 0 };
    ManagedVector<Token *> *m_tokens { nullptr };
};
}
