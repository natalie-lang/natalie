#pragma once

#include <string>

#include "natalie.hpp"
#include "natalie/token.hpp"

namespace Natalie {

struct Node : public gc {
    enum class Type {
        Array,
        Assignment,
        AttrAssign,
        Block,
        BlockPass,
        Break,
        Call,
        Class,
        Constant,
        Def,
        EvaluateToString,
        False,
        Hash,
        Identifier,
        If,
        Iter,
        InterpolatedString,
        Literal,
        LogicalAnd,
        LogicalOr,
        Module,
        MultipleAssignment,
        Next,
        Nil,
        NilSexp,
        Range,
        Return,
        Splat,
        StabbyProc,
        String,
        Symbol,
        True,
        Yield,
    };

    Node(Token token)
        : m_token { token } { }

    virtual Value *to_ruby(Env *env) {
        NAT_UNREACHABLE();
    }

    virtual Type type() { NAT_UNREACHABLE(); }

    const char *file() { return m_token.file(); }
    size_t line() { return m_token.line(); }
    size_t column() { return m_token.column(); }

private:
    Token m_token {};
};

struct NodeWithArgs : Node {
    NodeWithArgs(Token token)
        : Node { token } { }

    void add_arg(Node *arg) {
        m_args.push(arg);
    }

protected:
    Vector<Node *> m_args {};
};

struct ArrayNode : Node {
    ArrayNode(Token token)
        : Node { token } { }

    virtual Type type() override { return Type::Array; }

    virtual Value *to_ruby(Env *) override;

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    Vector<Node *> *nodes() { return &m_nodes; }

protected:
    Vector<Node *> m_nodes {};
};

struct BlockPassNode : Node {
    BlockPassNode(Token token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() override { return Type::BlockPass; }

    virtual Value *to_ruby(Env *) override;

private:
    Node *m_node { nullptr };
};

struct BreakNode : NodeWithArgs {
    BreakNode(Token token, Node *arg = nullptr)
        : NodeWithArgs { token }
        , m_arg { arg } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::Break; }

private:
    Node *m_arg { nullptr };
};

struct IdentifierNode;

struct AssignmentNode : Node {
    AssignmentNode(Token token, Node *identifier, Node *value)
        : Node { token }
        , m_identifier { identifier }
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
    BlockNode(Token token)
        : Node { token } { }

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

struct CallNode : NodeWithArgs {
    CallNode(Token token, Node *receiver, Value *message)
        : NodeWithArgs { token }
        , m_receiver { receiver }
        , m_message { message } {
        assert(m_receiver);
        assert(m_message);
    }

    CallNode(Token token, CallNode &node)
        : NodeWithArgs { token }
        , m_receiver { node.m_receiver }
        , m_message { node.m_message } {
        for (auto arg : node.m_args) {
            add_arg(arg);
        }
    }

    virtual Type type() override { return Type::Call; }

    virtual Value *to_ruby(Env *) override;

    Value *message() { return m_message; }
    void set_message(Value *message) { m_message = message; }

protected:
    Node *m_receiver { nullptr };
    Value *m_message { nullptr };
};

struct AttrAssignNode : CallNode {
    AttrAssignNode(Token token, Node *receiver, Value *message)
        : CallNode { token, receiver, message } { }

    AttrAssignNode(Token token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() override { return Type::AttrAssign; }

    virtual Value *to_ruby(Env *) override;
};

struct ConstantNode;

struct ClassNode : Node {
    ClassNode(Token token, ConstantNode *name, Node *superclass, BlockNode *body)
        : Node { token }
        , m_name { name }
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
        : Node { token }
        , m_token { token } { }

    virtual Type type() override { return Type::Constant; }

    virtual Value *to_ruby(Env *) override;

    const char *name() { return m_token.literal(); }

private:
    Token m_token {};
};

struct LiteralNode : Node {
    LiteralNode(Token token, Value *value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Literal; }

    virtual Value *to_ruby(Env *) override;

    Value *value() { return m_value; }
    Value::Type value_type() { return m_value->type(); }

private:
    Value *m_value { nullptr };
};

struct DefNode : Node {
    DefNode(Token token, IdentifierNode *name, Vector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_args { args }
        , m_body { body } { }

    virtual Type type() override { return Type::Def; }

    virtual Value *to_ruby(Env *) override;

private:
    SexpValue *build_args_sexp(Env *);

    IdentifierNode *m_name { nullptr };
    Vector<Node *> *m_args { nullptr };
    BlockNode *m_body { nullptr };
};

struct EvaluateToStringNode : Node {
    EvaluateToStringNode(Token token, Node *node)
        : Node { token }
        , m_node { node } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::EvaluateToString; }

private:
    Node *m_node { nullptr };
};

struct FalseNode : Node {
    FalseNode(Token token)
        : Node { token } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::False; }
};

struct HashNode : Node {
    HashNode(Token token)
        : Node { token } { }

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
        : Node { token }
        , m_token { token }
        , m_is_lvar { is_lvar } { }

    virtual Type type() override { return Type::Identifier; }

    virtual Value *to_ruby(Env *) override;

    Token::Type token_type() { return m_token.type(); }
    const char *name() { return m_token.literal(); }

    bool is_lvar() { return m_is_lvar; }
    void set_is_lvar(bool is_lvar) { m_is_lvar = is_lvar; }

    SexpValue *to_sexp(Env *);

    SymbolValue *assignment_type(Env *env) {
        switch (token_type()) {
        case Token::Type::BareName:
            return SymbolValue::intern(env, "lasgn");
        case Token::Type::ClassVariable:
            return SymbolValue::intern(env, "cvdecl");
        case Token::Type::Constant:
            return SymbolValue::intern(env, "cdecl");
        case Token::Type::GlobalVariable:
            return SymbolValue::intern(env, "gasgn");
        case Token::Type::InstanceVariable:
            return SymbolValue::intern(env, "iasgn");
        default:
            NAT_UNREACHABLE();
        }
    }

    SymbolValue *to_symbol(Env *env) {
        return SymbolValue::intern(env, name());
    }

private:
    Token m_token {};
    bool m_is_lvar { false };
};

struct IfNode : Node {
    IfNode(Token token, Node *condition, Node *true_expr, Node *false_expr)
        : Node { token }
        , m_condition { condition }
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
    IterNode(Token token, Node *call, Vector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_call { call }
        , m_args { args }
        , m_body { body } {
        assert(m_call);
        assert(m_args);
        assert(m_body);
    }

    virtual Type type() override { return Type::Iter; }

    virtual Value *to_ruby(Env *) override;

private:
    SexpValue *build_args_sexp(Env *);

    Node *m_call { nullptr };
    Vector<Node *> *m_args { nullptr };
    BlockNode *m_body { nullptr };
};

struct InterpolatedStringNode : Node {
    InterpolatedStringNode(Token token)
        : Node { token } { }

    virtual Type type() override { return Type::InterpolatedString; }

    virtual Value *to_ruby(Env *) override;

    void add_node(Node *node) { m_nodes.push(node); };

private:
    Vector<Node *> m_nodes {};
};

struct LogicalAndNode : Node {
    LogicalAndNode(Token token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    virtual Type type() override { return Type::LogicalAnd; }

    virtual Value *to_ruby(Env *) override;

private:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};

struct LogicalOrNode : Node {
    LogicalOrNode(Token token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    virtual Type type() override { return Type::LogicalOr; }

    virtual Value *to_ruby(Env *) override;

private:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};

struct ModuleNode : Node {
    ModuleNode(Token token, ConstantNode *name, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_body { body } { }

    virtual Type type() override { return Type::Module; }

    virtual Value *to_ruby(Env *) override;

private:
    ConstantNode *m_name { nullptr };
    BlockNode *m_body { nullptr };
};

struct MultipleAssignmentNode : ArrayNode {
    MultipleAssignmentNode(Token token)
        : ArrayNode { token } { }

    virtual Type type() override { return Type::MultipleAssignment; }

    virtual Value *to_ruby(Env *) override;
    ArrayValue *to_ruby_with_array(Env *);

    void add_locals(Env *env, Vector<SymbolValue *> *locals) {
        for (auto node : m_nodes) {
            switch (node->type()) {
            case Node::Type::Identifier: {
                auto identifier = static_cast<IdentifierNode *>(node);
                if (identifier->token_type() == Token::Type::BareName)
                    locals->push(SymbolValue::intern(env, identifier->name()));
                break;
            }
            case Node::Type::MultipleAssignment:
                static_cast<MultipleAssignmentNode *>(node)->add_locals(env, locals);
                break;
            default:
                NAT_UNREACHABLE();
            }
        }
    }
};

struct NextNode : Node {
    NextNode(Token token, Node *arg = nullptr)
        : Node { token }
        , m_arg { arg } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::Next; }

private:
    Node *m_arg { nullptr };
};

struct NilNode : Node {
    NilNode(Token token)
        : Node { token } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::Nil; }
};

struct NilSexpNode : Node {
    NilSexpNode(Token token)
        : Node { token } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::NilSexp; }
};

struct RangeNode : Node {
    RangeNode(Token token, Node *first, Node *last, bool exclude_end)
        : Node { token }
        , m_first { first }
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
    ReturnNode(Token token, Node *arg = nullptr)
        : Node { token }
        , m_arg { arg } { }

    virtual Type type() override { return Type::Return; }

    virtual Value *to_ruby(Env *) override;

private:
    Node *m_arg { nullptr };
};

struct SplatNode : Node {
    SplatNode(Token token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() override { return Type::Splat; }

    virtual Value *to_ruby(Env *) override;

private:
    Node *m_node { nullptr };
};

struct StabbyProcNode : Node {
    StabbyProcNode(Token token, Vector<Node *> *args)
        : Node { token }
        , m_args { args } {
        assert(m_args);
    }

    virtual Type type() override { return Type::StabbyProc; }

    virtual Value *to_ruby(Env *) override;

    Vector<Node *> *args() { return m_args; };

private:
    Vector<Node *> *m_args { nullptr };
};

struct StringNode : Node {
    StringNode(Token token, Value *value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::String; }

    virtual Value *to_ruby(Env *) override;

    Value *value() { return m_value; }

private:
    Value *m_value { nullptr };
};

struct SymbolNode : Node {
    SymbolNode(Token token, Value *value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Symbol; }

    virtual Value *to_ruby(Env *) override;

private:
    Value *m_value { nullptr };
};

struct TrueNode : Node {
    TrueNode(Token token)
        : Node { token } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::True; }
};

struct YieldNode : NodeWithArgs {
    YieldNode(Token token)
        : NodeWithArgs { token } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::Yield; }
};

}
