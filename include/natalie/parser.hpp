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
        Node() { }

        size_t line() { return m_line; }
        size_t column() { return m_column; }

        virtual Value *to_ruby(Env *env) {
            return env->nil_obj();
        }

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

        virtual Value *to_ruby(Env *env) override {
            const char *assignment_type;
            switch (m_identifier->token_type()) {
            case Token::Type::ClassVariable:
                assignment_type = "cvdecl";
                break;
            case Token::Type::Constant:
                assignment_type = "cdecl";
                break;
            case Token::Type::GlobalVariable:
                assignment_type = "gasgn";
                break;
            case Token::Type::Identifier:
                assignment_type = "lasgn";
                break;
            case Token::Type::InstanceVariable:
                assignment_type = "iasgn";
                break;
            default:
                NAT_UNREACHABLE();
            }
            return new SexpValue { env, {
                                            SymbolValue::intern(env, assignment_type),
                                            SymbolValue::intern(env, m_identifier->name()),
                                            m_value->to_ruby(env),
                                        } };
        }

    private:
        IdentifierNode *m_identifier { nullptr };
        Node *m_value { nullptr };
    };

    struct BlockNode : Node {
        void add_node(Node *node) {
            m_nodes.push(node);
        }

        virtual Value *to_ruby(Env *env) override {
            auto *array = new SexpValue { env, { SymbolValue::intern(env, "block") } };
            for (auto node : m_nodes) {
                array->push(node->to_ruby(env));
            }
            return array;
        }

    private:
        Vector<Node *> m_nodes {};
    };

    struct CallNode : Node {
        CallNode(Node *receiver, Value *message, Node *arg)
            : m_receiver { receiver }
            , m_message { message }
            , m_arg { arg } {
            assert(m_receiver);
            assert(m_message);
            assert(m_arg);
        }

        virtual Value *to_ruby(Env *env) override {
            return new SexpValue { env, {
                                            SymbolValue::intern(env, "call"),
                                            m_receiver->to_ruby(env),
                                            m_message,
                                            m_arg->to_ruby(env),
                                        } };
        }

    private:
        Node *m_receiver { nullptr };
        Value *m_message { nullptr };
        Node *m_arg { nullptr };
    };

    struct IdentifierNode : Node {
        IdentifierNode(Token token)
            : m_token { token } { }

        virtual Value *to_ruby(Env *env) override {
            return new StringValue { env, "FIXME: IdentifierNode#to_ruby" };
        }

        Token::Type token_type() { return m_token.type(); }
        const char *name() { return m_token.literal(); }

    private:
        Token m_token {};
    };

    struct LiteralNode : Node {
        LiteralNode(Value *value)
            : m_value { value } {
            assert(m_value);
        }

        virtual Value *to_ruby(Env *env) override {
            return new SexpValue { env, { SymbolValue::intern(env, "lit"), m_value } };
        }

    private:
        Value *m_value { nullptr };
    };

    struct SymbolNode : Node {
        SymbolNode(Value *value)
            : m_value { value } {
            assert(m_value);
        }

        virtual Value *to_ruby(Env *env) override {
            return new SexpValue { env, { SymbolValue::intern(env, "lit"), m_value } };
        }

    private:
        Value *m_value { nullptr };
    };

    struct StringNode : Node {
        StringNode(Value *value)
            : m_value { value } {
            assert(m_value);
        }

        virtual Value *to_ruby(Env *env) override {
            return new SexpValue { env, { SymbolValue::intern(env, "str"), m_value } };
        }

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
        PREFIX,
        CALL,
    };

    Precedence get_precedence(Token token) {
        switch (token.type()) {
        case Token::Type::Plus:
        case Token::Type::Minus:
            return SUM;
        case Token::Type::Multiply:
        case Token::Type::Divide:
            return PRODUCT;
        case Token::Type::Equal:
            return ASSIGNMENT;
        default:
            return LOWEST;
        }
    }

    Node *parse_expression(Env *env, Precedence precedence = LOWEST) {
        skip_newlines();

        auto null_fn = null_denotation(current_token().type());
        if (!null_fn) {
            raise_unexpected(env);
        }

        Node *left = (this->*null_fn)(env);

        while (current_token().is_valid() && precedence < get_precedence(current_token())) {
            auto left_fn = left_denotation(current_token().type());
            if (!left_fn) {
                raise_unexpected(env);
            }

            left = (this->*left_fn)(env, left);
        }
        return left;
    }

    Node *tree(Env *env) {
        auto tree = new BlockNode {};
        current_token().validate(env);
        while (!current_token().is_eof()) {
            auto exp = parse_expression(env);
            tree->add_node(exp);
            current_token().validate(env);
            skip_semicolons();
        }
        return tree;
    }

private:
    Node *parse_lit(Env *env) {
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

    Node *parse_string(Env *env) {
        auto lit = new StringNode { new StringValue { env, current_token().literal() } };
        advance();
        return lit;
    };

    Node *parse_identifier(Env *env) {
        auto identifier = new IdentifierNode { current_token() };
        advance();
        return identifier;
    };

    Node *parse_infix_expression(Env *env, Node *left) {
        auto op = current_token();
        advance();
        auto right = parse_expression(env, get_precedence(op));
        return new CallNode {
            left,
            op.type_value(env),
            right,
        };
    };

    Node *parse_assignment_expression(Env *env, Node *left) {
        advance();
        auto right = parse_expression(env, ASSIGNMENT);
        return new AssignmentNode {
            static_cast<IdentifierNode *>(left),
            right,
        };
    };

    Node *parse_grouped_expression(Env *env) {
        advance();
        auto exp = parse_expression(env, LOWEST);
        skip_newlines();
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

    using parse_fn1 = Node *(Parser::*)(Env *);
    using parse_fn2 = Node *(Parser::*)(Env *, Node *);

    parse_fn1 null_denotation(Token::Type type) {
        using Type = Token::Type;
        switch (type) {
        case Type::Integer:
        case Type::Float:
            return &Parser::parse_lit;
        case Type::ClassVariable:
        case Type::Constant:
        case Type::GlobalVariable:
        case Type::Identifier:
        case Type::InstanceVariable:
            return &Parser::parse_identifier;
        case Type::String:
            return &Parser::parse_string;
        case Type::LParen:
            return &Parser::parse_grouped_expression;
        default:
            return nullptr;
        }
    };

    parse_fn2 left_denotation(Token::Type type) {
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

    Token current_token() {
        if (m_index < m_tokens->size()) {
            return (*m_tokens)[m_index];
        } else {
            return Token::invalid();
        }
    }

    Token peek_token() {
        if (m_index + 1 < m_tokens->size()) {
            return (*m_tokens)[m_index + 1];
        } else {
            return Token::invalid();
        }
    }

    void skip_newlines() {
        while (current_token().is_eol())
            advance();
    }

    void skip_semicolons() {
        while (current_token().is_semicolon())
            advance();
    }

    void raise_unexpected(Env *env) {
        auto line = current_token().line() + 1;
        auto type = current_token().type_value(env);
        if (type == SymbolValue::intern(env, "EOF")) {
            env->raise("SyntaxError", "%d: syntax error, unexpected end-of-input", line);
        } else {
            env->raise("SyntaxError", "%d: syntax error, unexpected '%s'", line, type->c_str());
        }
    }

    void advance() { m_index++; }

    const char *m_code { nullptr };
    size_t m_index { 0 };
    Vector<Token> *m_tokens { nullptr };
};

}
