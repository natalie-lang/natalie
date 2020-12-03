#pragma once

#include <functional>
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

    Precedence get_precedence(Lexer::Token::Type type) {
        switch (type) {
        case Lexer::Token::Type::Plus:
        case Lexer::Token::Type::Minus:
            return SUM;
        case Lexer::Token::Type::Multiply:
        case Lexer::Token::Type::Divide:
            return PRODUCT;
        default:
            NAT_UNREACHABLE();
        }
    }

    Node *parse_expression(Env *env, Precedence precedence = LOWEST) {
        auto parse_integer = [env, this]() {
            auto lit = new LiteralNode { new IntegerValue { env, current_token().integer() } };
            advance();
            return lit;
        };

        auto parse_string = [env, this]() {
            auto lit = new StringNode { new StringValue { env, current_token().literal() } };
            advance();
            return lit;
        };

        auto parse_infix_expression = [env, this](Node *left) {
            auto op = current_token();
            advance();
            auto right = parse_expression(env, get_precedence(op.type()));
            return new CallNode {
                left,
                op.type_value(env),
                right,
            };
        };

        auto null_denotation = [&](Lexer::Token::Type type) -> std::function<Node *()> {
            using Type = Lexer::Token::Type;
            switch (type) {
            case Type::Integer:
                return parse_integer;
            case Type::String:
                return parse_string;
            default:
                NAT_UNREACHABLE();
            }
        };

        auto left_denotation = [parse_infix_expression](Lexer::Token::Type type) -> std::function<Node *(Node *)> {
            return parse_infix_expression;
        };

        auto null_fn = null_denotation(current_token().type());
        Node *left = null_fn();

        while (current_token().is_valid() && precedence < get_precedence(current_token().type())) {
            auto left_fn = left_denotation(current_token().type());
            left = left_fn(left);
        }
        return left;
    }

    Node *tree(Env *env) {
        auto tree = new BlockNode {};
        if (current_token().is_valid()) {
            auto exp = parse_expression(env);
            tree->add_node(exp);
        }
        return tree;
    }

private:
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

    void advance() { m_index++; }

    const char *m_code { nullptr };
    size_t m_index { 0 };
    Vector<Lexer::Token> *m_tokens { nullptr };
};

}
