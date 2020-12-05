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

    struct BlockNode : Node {
        void add_node(Node *node) {
            m_nodes.push(node);
        }

        virtual Value *to_ruby(Env *env) override {
            auto *array = new ArrayValue { env, { SymbolValue::intern(env, "block") } };
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
            return new ArrayValue { env, {
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

    struct LiteralNode : Node {
        LiteralNode(Value *value)
            : m_value { value } {
            assert(m_value);
        }

        virtual Value *to_ruby(Env *env) override {
            return new ArrayValue { env, { SymbolValue::intern(env, "lit"), m_value } };
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
            return new ArrayValue { env, { SymbolValue::intern(env, "lit"), m_value } };
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
            return new ArrayValue { env, { SymbolValue::intern(env, "str"), m_value } };
        }

    private:
        Value *m_value { nullptr };
    };

    enum Precedence {
        LOWEST = 1,
        EQUALITY = 2,
        LESSGREATER = 3,
        SUM = 4,
        PRODUCT = 5,
        PREFIX = 6,
        CALL = 7,
    };

    Precedence get_precedence(Env *env, Lexer::Token token) {
        switch (token.type()) {
        case Lexer::Token::Type::Plus:
        case Lexer::Token::Type::Minus:
            return SUM;
        case Lexer::Token::Type::Multiply:
        case Lexer::Token::Type::Divide:
            return PRODUCT;
        default:
            return LOWEST;
        }
    }

    Node *parse_expression(Env *env, Precedence precedence = LOWEST) {
        skip_newlines();

        auto null_fn = null_denotation(current_token().type());
        Node *left = (this->*null_fn)(env);

        while (current_token().is_valid() && precedence < get_precedence(env, current_token())) {
            auto left_fn = left_denotation(current_token().type());
            left = (this->*left_fn)(env, left);
        }
        return left;
    }

    Node *tree(Env *env) {
        auto tree = new BlockNode {};
        if (!current_token().is_valid()) {
            fprintf(stderr, "Invalid token: %s\n", current_token().literal());
            abort();
        }
        while (!current_token().is_eof()) {
            auto exp = parse_expression(env);
            tree->add_node(exp);
            if (!current_token().is_valid()) {
                fprintf(stderr, "Invalid token: %s\n", current_token().literal());
                abort();
            }
            skip_semicolons();
        }
        return tree;
    }

private:
    Node *parse_integer(Env *env) {
        auto lit = new LiteralNode { new IntegerValue { env, current_token().integer() } };
        advance();
        return lit;
    };

    Node *parse_string(Env *env) {
        auto lit = new StringNode { new StringValue { env, current_token().literal() } };
        advance();
        return lit;
    };

    Node *parse_infix_expression(Env *env, Node *left) {
        auto op = current_token();
        advance();
        auto right = parse_expression(env, get_precedence(env, op));
        return new CallNode {
            left,
            op.type_value(env),
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
        } else if (current_token().type() != Lexer::Token::Type::RParen) {
            fprintf(stderr, "expected ), but got %s\n", current_token().type_value(env)->inspect(env));
            abort();
        }
        advance();
        return exp;
    };

    using parse_fn1 = Node *(Parser::*)(Env *);
    using parse_fn2 = Node *(Parser::*)(Env *, Node *);

    parse_fn1 null_denotation(Lexer::Token::Type type) {
        using Type = Lexer::Token::Type;
        switch (type) {
        case Type::Integer:
            return &Parser::parse_integer;
        case Type::String:
            return &Parser::parse_string;
        case Type::LParen:
            return &Parser::parse_grouped_expression;
        default:
            NAT_UNREACHABLE();
        }
    };

    parse_fn2 left_denotation(Lexer::Token::Type type) {
        return &Parser::parse_infix_expression;
    };

    Lexer::Token current_token() {
        if (m_index < m_tokens->size()) {
            return (*m_tokens)[m_index];
        } else {
            return Lexer::Token::invalid();
        }
    }

    Lexer::Token peek_token() {
        if (m_index + 1 < m_tokens->size()) {
            return (*m_tokens)[m_index + 1];
        } else {
            return Lexer::Token::invalid();
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

    void advance() { m_index++; }

    const char *m_code { nullptr };
    size_t m_index { 0 };
    Vector<Lexer::Token> *m_tokens { nullptr };
};

}
