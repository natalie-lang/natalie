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
            Assignment,
            Block,
            Call,
            Def,
            Identifier,
            Literal,
            Symbol,
            String,
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

    struct IdentifierNode;

    struct AssignmentNode : Node {
        AssignmentNode(IdentifierNode *identifier, Node *value)
            : m_identifier { identifier }
            , m_value { value } {
            assert(m_identifier);
            assert(m_value);
        }

        virtual Type type() override { return Type::Assignment; }

        virtual Value *to_ruby(Env *) override;

        const char *name() { return m_identifier->name(); }

    private:
        IdentifierNode *m_identifier { nullptr };
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

    struct IdentifierNode : Node {
        IdentifierNode(Token token, bool is_lvar)
            : m_token { token }
            , m_is_lvar { is_lvar } { }

        virtual Type type() override { return Type::Identifier; }

        virtual Value *to_ruby(Env *) override;

        Token::Type token_type() { return m_token.type(); }
        const char *name() { return m_token.literal(); }

    private:
        Token m_token {};
        bool m_is_lvar { false };
    };

    struct LiteralNode : Node {
        LiteralNode(Value *value)
            : m_value { value } {
            assert(m_value);
        }

        virtual Type type() override { return Type::Literal; }

        virtual Value *to_ruby(Env *) override;

    private:
        Value *m_value { nullptr };
    };

    struct NilNode : Node {
        virtual Value *to_ruby(Env *) override;
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

    enum Precedence {
        LOWEST,
        ASSIGNMENT,
        EQUALITY,
        LESSGREATER,
        SUM,
        PRODUCT,
        DOT,
        PREFIX,
        CALL,
    };

    Node *tree(Env *);

private:
    Precedence get_precedence() {
        switch (current_token().type()) {
        case Token::Type::Plus:
        case Token::Type::Minus:
            return SUM;
        case Token::Type::Multiply:
        case Token::Type::Divide:
            return PRODUCT;
        case Token::Type::Equal:
            return ASSIGNMENT;
        case Token::Type::LParen:
            return CALL;
        case Token::Type::Dot:
            return DOT;
        default:
            return LOWEST;
        }
    }

    using LocalsVectorPtr = Vector<SymbolValue *> *;

    Node *parse_expression(Env *, Precedence, LocalsVectorPtr);

    Node *parse_body(Env *, LocalsVectorPtr);
    Node *parse_def(Env *, LocalsVectorPtr);
    Node *parse_identifier(Env *, LocalsVectorPtr);
    Node *parse_lit(Env *, LocalsVectorPtr);
    Node *parse_string(Env *, LocalsVectorPtr);

    Node *parse_assignment_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_call_expression_without_parens(Env *, Node *, LocalsVectorPtr);
    Node *parse_call_expression_with_parens(Env *, Node *, LocalsVectorPtr);
    Node *parse_grouped_expression(Env *, LocalsVectorPtr);
    Node *parse_infix_expression(Env *, Node *, LocalsVectorPtr);
    Node *parse_send_expression(Env *, Node *, LocalsVectorPtr);

    using parse_null_fn = Node *(Parser::*)(Env *, LocalsVectorPtr);
    using parse_left_fn = Node *(Parser::*)(Env *, Node *, LocalsVectorPtr);

    parse_null_fn null_denotation(Token::Type);
    parse_left_fn left_denotation(Token::Type);

    Token current_token();
    Token peek_token();

    void next_expression(Env *);
    void skip_newlines();

    void raise_unexpected(Env *, const char *);

    void advance() { m_index++; }

    const char *m_code { nullptr };
    size_t m_index { 0 };
    Vector<Token> *m_tokens { nullptr };
};

}
