#pragma once

#include <string>

#include "natalie.hpp"
#include "natalie/lexer.hpp"

namespace Natalie {

struct Parser : public gc {
    Parser(const char *code)
        : m_code { code } {
        assert(m_code);
        m_tokens = Lexer { m_code }.tokens();
    }

    struct Node : public gc {
        enum class Type {
            Array,
            Assignment,
            Block,
            BlockPass,
            Call,
            Class,
            CommaSeparatedIdentifiers,
            Constant,
            Def,
            False,
            Hash,
            Identifier,
            If,
            Iter,
            Literal,
            Module,
            Nil,
            Range,
            Return,
            Symbol,
            String,
            True,
        };

        Node() { }

        size_t line() { return m_line; }
        size_t column() { return m_column; }

        virtual Value *to_ruby(Env *env) {
            NAT_UNREACHABLE();
        }

        virtual Type type() { NAT_UNREACHABLE(); }

    private:
        size_t m_line { 0 };
        size_t m_column { 0 };
    };

    struct ArrayNode : Node {
        ArrayNode() { }

        virtual Type type() override { return Type::Array; }

        virtual Value *to_ruby(Env *) override;

        void add_node(Node *node) {
            m_nodes.push(node);
        }

        Vector<Node *> *nodes() { return &m_nodes; }

    private:
        Vector<Node *> m_nodes {};
    };

    struct BlockPassNode : Node {
        BlockPassNode(Node *node)
            : m_node { node } {
            assert(m_node);
        }

        virtual Type type() override { return Type::BlockPass; }

        virtual Value *to_ruby(Env *) override;

    private:
        Node *m_node { nullptr };
    };

    struct CommaSeparatedIdentifiersNode : ArrayNode {
        virtual Type type() override { return Type::CommaSeparatedIdentifiers; }
    };

    struct IdentifierNode;

    struct AssignmentNode : Node {
        AssignmentNode(Node *identifier, Node *value)
            : m_identifier { identifier }
            , m_value { value } {
            assert(m_identifier);
            assert(m_value);
        }

        virtual Type type() override { return Type::Assignment; }

        virtual Value *to_ruby(Env *) override;

    private:
        Node *m_identifier { nullptr };
        Node *m_value { nullptr };
    };

    struct BlockNode : Node {
        void add_node(Node *node) {
            m_nodes.push(node);
        }

        virtual Type type() override { return Type::Block; }

        virtual Value *to_ruby(Env *) override;

        Vector<Node *> *nodes() { return &m_nodes; }
        bool is_empty() { return m_nodes.is_empty(); }

        bool has_one_node() { return m_nodes.size() == 1; }

    private:
        Vector<Node *> m_nodes {};
    };

    struct CallNode : Node {
        CallNode(Node *receiver, Value *message)
            : m_receiver { receiver }
            , m_message { message } {
            assert(m_receiver);
            assert(m_message);
        }

        virtual Type type() override { return Type::Call; }

        virtual Value *to_ruby(Env *) override;

        void add_arg(Node *arg) {
            m_args.push(arg);
        }

    private:
        Node *m_receiver { nullptr };
        Value *m_message { nullptr };
        Vector<Node *> m_args {};
    };

    struct ConstantNode;

    struct ClassNode : Node {
        ClassNode(ConstantNode *name, Node *superclass, BlockNode *body)
            : m_name { name }
            , m_superclass { superclass }
            , m_body { body } { }

        virtual Type type() override { return Type::Class; }

        virtual Value *to_ruby(Env *) override;

    private:
        ConstantNode *m_name { nullptr };
        Node *m_superclass { nullptr };
        BlockNode *m_body { nullptr };
    };

    struct ConstantNode : Node {
        ConstantNode(Token token)
            : m_token { token } { }

        virtual Type type() override { return Type::Constant; }

        virtual Value *to_ruby(Env *) override;

        const char *name() { return m_token.literal(); }

    private:
        Token m_token {};
    };

    struct LiteralNode;

    struct DefNode : Node {
        DefNode(IdentifierNode *name, Vector<Node *> *args, BlockNode *body)
            : m_name { name }
            , m_args { args }
            , m_body { body } { }

        virtual Type type() override { return Type::Def; }

        virtual Value *to_ruby(Env *) override;

    private:
        SexpValue *build_args_sexp(Env *env) {
            auto sexp = new SexpValue { env, { SymbolValue::intern(env, "args") } };
            for (auto arg : *m_args) {
                switch (arg->type()) {
                case Node::Type::Identifier:
                    sexp->push(SymbolValue::intern(env, static_cast<IdentifierNode *>(arg)->name()));
                    break;
                default:
                    NAT_UNREACHABLE();
                }
            }
            return sexp;
        }

        IdentifierNode *m_name { nullptr };
        Vector<Node *> *m_args { nullptr };
        BlockNode *m_body { nullptr };
    };

    struct FalseNode : Node {
        virtual Value *to_ruby(Env *) override;

        virtual Type type() override { return Type::False; }
    };

    struct HashNode : Node {
        HashNode() { }

        virtual Type type() override { return Type::Array; }

        virtual Value *to_ruby(Env *) override;

        void add_node(Node *node) {
            m_nodes.push(node);
        }

    private:
        Vector<Node *> m_nodes {};
    };

    struct IdentifierNode : Node {
        IdentifierNode(Token token, bool is_lvar)
            : m_token { token }
            , m_is_lvar { is_lvar } { }

        virtual Type type() override { return Type::Identifier; }

        virtual Value *to_ruby(Env *) override;

        Token::Type token_type() { return m_token.type(); }
        const char *name() { return m_token.literal(); }

        void set_is_lvar(bool is_lvar) { m_is_lvar = is_lvar; }

        SexpValue *to_sexp(Env *env) {
            return new SexpValue {
                env, { assignment_type(env), SymbolValue::intern(env, name()) }
            };
        }

        SymbolValue *assignment_type(Env *env) {
            switch (token_type()) {
            case Token::Type::ClassVariable:
                return SymbolValue::intern(env, "cvdecl");
            case Token::Type::Constant:
                return SymbolValue::intern(env, "cdecl");
            case Token::Type::GlobalVariable:
                return SymbolValue::intern(env, "gasgn");
            case Token::Type::Identifier:
                return SymbolValue::intern(env, "lasgn");
            case Token::Type::InstanceVariable:
                return SymbolValue::intern(env, "iasgn");
            default:
                NAT_UNREACHABLE();
            }
        }

    private:
        Token m_token {};
        bool m_is_lvar { false };
    };

    struct IfNode : Node {
        IfNode(Node *condition, Node *true_expr, Node *false_expr)
            : m_condition { condition }
            , m_true_expr { true_expr }
            , m_false_expr { false_expr } {
            assert(m_condition);
            assert(m_true_expr);
            assert(m_false_expr);
        }

        virtual Type type() override { return Type::If; }

        virtual Value *to_ruby(Env *) override;

    private:
        Node *m_condition { nullptr };
        Node *m_true_expr { nullptr };
        Node *m_false_expr { nullptr };
    };

    struct IterNode : Node {
        IterNode(Node *call, Vector<Node *> *args, BlockNode *body)
            : m_call { call }
            , m_args { args }
            , m_body { body } {
            assert(m_call);
            assert(m_args);
            assert(m_body);
        }

        virtual Type type() override { return Type::Iter; }

        virtual Value *to_ruby(Env *) override;

    private:
        SexpValue *build_args_sexp(Env *env) {
            auto sexp = new SexpValue { env, { SymbolValue::intern(env, "args") } };
            for (auto arg : *m_args) {
                switch (arg->type()) {
                case Node::Type::Identifier:
                    sexp->push(SymbolValue::intern(env, static_cast<IdentifierNode *>(arg)->name()));
                    break;
                default:
                    NAT_UNREACHABLE();
                }
            }
            return sexp;
        }

        Node *m_call { nullptr };
        Vector<Node *> *m_args { nullptr };
        BlockNode *m_body { nullptr };
    };

    struct LiteralNode : Node {
        LiteralNode(Value *value)
            : m_value { value } {
            assert(m_value);
        }

        virtual Type type() override { return Type::Literal; }

        virtual Value *to_ruby(Env *) override;

        Value *value() { return m_value; }
        Value::Type value_type() { return m_value->type(); }

    private:
        Value *m_value { nullptr };
    };

    struct ModuleNode : Node {
        ModuleNode(ConstantNode *name, BlockNode *body)
            : m_name { name }
            , m_body { body } { }

        virtual Type type() override { return Type::Module; }

        virtual Value *to_ruby(Env *) override;

    private:
        ConstantNode *m_name { nullptr };
        BlockNode *m_body { nullptr };
    };

    struct NilNode : Node {
        virtual Value *to_ruby(Env *) override;

        virtual Type type() override { return Type::Nil; }
    };

    struct RangeNode : Node {
        RangeNode(Node *first, Node *last, bool exclude_end)
            : m_first { first }
            , m_last { last }
            , m_exclude_end { exclude_end } {
            assert(m_first);
            assert(m_last);
        }

        virtual Type type() override { return Type::Range; }

        virtual Value *to_ruby(Env *) override;

    private:
        Node *m_first { nullptr };
        Node *m_last { nullptr };
        bool m_exclude_end { false };
    };

    struct ReturnNode : Node {
        ReturnNode() { }

        ReturnNode(Node *node)
            : m_node { node } {
            assert(m_node);
        }

        virtual Type type() override { return Type::Return; }

        virtual Value *to_ruby(Env *) override;

    private:
        Node *m_node { nullptr };
    };

    struct SymbolNode : Node {
        SymbolNode(Value *value)
            : m_value { value } {
            assert(m_value);
        }

        virtual Type type() override { return Type::Symbol; }

        virtual Value *to_ruby(Env *) override;

    private:
        Value *m_value { nullptr };
    };

    struct StringNode : Node {
        StringNode(Value *value)
            : m_value { value } {
            assert(m_value);
        }

        virtual Type type() override { return Type::String; }

        virtual Value *to_ruby(Env *) override;

    private:
        Value *m_value { nullptr };
    };

    struct TrueNode : Node {
        virtual Value *to_ruby(Env *) override;

        virtual Type type() override { return Type::True; }
    };

    enum Precedence {
        LOWEST,
        ARRAY,
        HASH,
        ITER,
        TERNARY,
        ASSIGNMENT,
        RANGE,
        EQUALITY,
        LESSGREATER,
        BITWISEOR,
        BITWISEAND,
        SUM,
        PRODUCT,
        DOT,
        PREFIX,
        EXPONENT,
        CALL,
    };

    Node *tree(Env *);

private:
    Precedence get_precedence() {
        switch (current_token().type()) {
        case Token::Type::Plus:
        case Token::Type::Minus:
            return SUM;
        case Token::Type::Integer:
        case Token::Type::Float:
            if (current_token().has_sign())
                return SUM;
            else
                return LOWEST;
        case Token::Type::Equal:
            return ASSIGNMENT;
        case Token::Type::BitwiseAnd:
            return BITWISEAND;
        case Token::Type::BitwiseOr:
        case Token::Type::BitwiseXor:
            return BITWISEOR;
        case Token::Type::LParen:
            return CALL;
        case Token::Type::Dot:
            return DOT;
        case Token::Type::EqualEqual:
            return EQUALITY;
        case Token::Type::Exponent:
            return EXPONENT;
        case Token::Type::DoKeyword:
        case Token::Type::LCurlyBrace:
            return ITER;
        case Token::Type::LessThan:
        case Token::Type::LessThanOrEqual:
        case Token::Type::GreaterThan:
        case Token::Type::GreaterThanOrEqual:
            return LESSGREATER;
        case Token::Type::Multiply:
        case Token::Type::Divide:
            return PRODUCT;
        case Token::Type::DotDot:
        case Token::Type::DotDotDot:
            return RANGE;
        case Token::Type::TernaryQuestion:
        case Token::Type::TernaryColon:
            return TERNARY;
        default:
            return LOWEST;
        }
    }

    using LocalsVectorPtr = Vector<SymbolValue *> *;

    Node *parse_expression(Env *, Precedence, LocalsVectorPtr);

    BlockNode *parse_body(Env *, LocalsVectorPtr, Precedence, Token::Type = Token::Type::EndKeyword);
    Node *parse_if_body(Env *, LocalsVectorPtr);

    Node *parse_array(Env *, LocalsVectorPtr);
    Node *parse_block_pass(Env *, LocalsVectorPtr);
    Node *parse_bool(Env *, LocalsVectorPtr);
    Node *parse_class(Env *, LocalsVectorPtr);
    Node *parse_comma_separated_identifiers(Env *, LocalsVectorPtr);
    Node *parse_constant(Env *, LocalsVectorPtr);
    Node *parse_def(Env *, LocalsVectorPtr);
    Vector<Node *> *parse_def_args(Env *, LocalsVectorPtr);
    Node *parse_group(Env *, LocalsVectorPtr);
    Node *parse_hash(Env *, LocalsVectorPtr);
    Node *parse_identifier(Env *, LocalsVectorPtr);
    Node *parse_if(Env *, LocalsVectorPtr);
    Node *parse_lit(Env *, LocalsVectorPtr);
    Node *parse_module(Env *, LocalsVectorPtr);
    Node *parse_return(Env *, LocalsVectorPtr);
    Node *parse_string(Env *, LocalsVectorPtr);
    Node *parse_symbol(Env *, LocalsVectorPtr);
    Node *parse_word_array(Env *, LocalsVectorPtr);
    Node *parse_word_symbol_array(Env *, LocalsVectorPtr);

    Node *parse_assignment_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_call_expression_without_parens(Env *, Node *, LocalsVectorPtr);
    Node *parse_call_expression_with_parens(Env *, Node *, LocalsVectorPtr);
    Node *parse_infix_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_iter_expression(Env *, Node *, LocalsVectorPtr);
    Vector<Node *> *parse_iter_args(Env *, LocalsVectorPtr);
    Node *parse_range_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_send_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_ternary_expression(Env *, Node *, LocalsVectorPtr);

    using parse_null_fn = Node *(Parser::*)(Env *, LocalsVectorPtr);
    using parse_left_fn = Node *(Parser::*)(Env *, Node *, LocalsVectorPtr);

    parse_null_fn null_denotation(Token::Type, Precedence);
    parse_left_fn left_denotation(Token::Type);

    Token current_token();
    Token peek_token();

    void next_expression(Env *);
    void skip_newlines();

    void expect(Env *, Token::Type, const char *);
    void raise_unexpected(Env *, const char *);

    void advance() { m_index++; }

    const char *m_code { nullptr };
    size_t m_index { 0 };
    Vector<Token> *m_tokens { nullptr };
};
}
