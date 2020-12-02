#pragma once

#include <string>

#include "natalie.hpp"
#include "natalie/lexer.hpp"

namespace Natalie {

struct Parser : public gc {
    Parser(const char *code)
        : m_code { code } {
        assert(m_code);
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

    Node *tree(Env *env) {
        auto *tree = new BlockNode {};
        auto tokens = Lexer { m_code }.tokens();
        for (auto token : *tokens) {
            switch (token.type()) {
            case Lexer::Token::Type::Integer:
                tree->add_node(new LiteralNode { new IntegerValue { env, token.integer() } });
                break;
            case Lexer::Token::Type::String:
                tree->add_node(new StringNode { new StringValue { env, token.literal() } });
                break;
            default:
                NAT_UNREACHABLE();
            }
        }
        return tree;
    }

private:
    const char *m_code { nullptr };
};

}
