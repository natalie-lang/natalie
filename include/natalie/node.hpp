#pragma once

#include <string>

#include "natalie.hpp"
#include "natalie/token.hpp"

namespace Natalie {

struct Node : public gc {
    enum class Type {
        Arg,
        Array,
        Assignment,
        AttrAssign,
        Begin,
        BeginRescue,
        Block,
        BlockPass,
        Break,
        Call,
        Case,
        CaseWhen,
        Class,
        Colon2,
        Colon3,
        Constant,
        Def,
        EvaluateToString,
        False,
        Hash,
        Identifier,
        If,
        Iter,
        InterpolatedShell,
        InterpolatedString,
        Literal,
        LogicalAnd,
        LogicalOr,
        Module,
        MultipleAssignment,
        Next,
        Nil,
        NilSexp,
        Not,
        OpAssignAnd,
        OpAssignOr,
        Range,
        Regexp,
        Return,
        SafeCall,
        Self,
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

    virtual bool is_callable() { return false; }

    const char *file() { return m_token.file(); }
    size_t line() { return m_token.line(); }
    size_t column() { return m_token.column(); }

    Token token() { return m_token; }

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

struct ArgNode : Node {
    ArgNode(Token token)
        : Node { token } { }

    ArgNode(Token token, const char *name)
        : Node { token }
        , m_name { name } {
        assert(m_name);
    }

    virtual Type type() override { return Type::Arg; }

    virtual Value *to_ruby(Env *) override;

    const char *name() { return m_name; }

    bool splat() { return m_splat; }
    void set_splat(bool splat) { m_splat = splat; }

protected:
    const char *m_name { nullptr };
    bool m_splat { false };
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

struct BlockNode;
struct BeginRescueNode;

struct BeginNode : Node {
    BeginNode(Token token, BlockNode *body)
        : Node { token }
        , m_body { body } {
        assert(m_body);
    }

    virtual Type type() override { return Type::Begin; }

    virtual Value *to_ruby(Env *) override;

    void add_rescue_node(BeginRescueNode *node) { m_rescue_nodes.push(node); }

    void set_else_body(BlockNode *else_body) { m_else_body = else_body; }
    void set_ensure_body(BlockNode *ensure_body) { m_ensure_body = ensure_body; }

private:
    BlockNode *m_body { nullptr };
    BlockNode *m_else_body { nullptr };
    BlockNode *m_ensure_body { nullptr };
    Vector<BeginRescueNode *> m_rescue_nodes {};
};

struct BeginRescueNode : Node {
    BeginRescueNode(Token token)
        : Node { token } { }

    virtual Type type() override { return Type::BeginRescue; }

    virtual Value *to_ruby(Env *) override;

    void add_exception_node(Node *node) {
        m_exceptions.push(node);
    }

    void set_exception_name(IdentifierNode *name) {
        m_name = name;
    }

    void set_body(BlockNode *body) { m_body = body; }

    Node *name_to_node();

private:
    IdentifierNode *m_name { nullptr };
    Vector<Node *> m_exceptions {};
    BlockNode *m_body { nullptr };
};

struct BlockNode : Node {
    BlockNode(Token token)
        : Node { token } { }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    virtual Type type() override { return Type::Block; }

    virtual Value *to_ruby(Env *) override;
    Value *to_ruby_with_name(Env *, const char *);

    Vector<Node *> *nodes() { return &m_nodes; }
    bool is_empty() { return m_nodes.is_empty(); }

    bool has_one_node() { return m_nodes.size() == 1; }

    Node *without_unnecessary_nesting() {
        if (has_one_node())
            return m_nodes[0];
        else
            return this;
    }

private:
    Vector<Node *> m_nodes {};
};

struct CallNode : NodeWithArgs {
    CallNode(Token token, Node *receiver, const char *message)
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

    virtual bool is_callable() override { return true; }

    const char *message() { return m_message; }
    void set_message(const char *message) { m_message = message; }

protected:
    Node *m_receiver { nullptr };
    const char *m_message { nullptr };
};

struct CaseNode : Node {
    CaseNode(Token token, Node *subject)
        : Node { token }
        , m_subject { subject } {
        assert(m_subject);
    }

    virtual Type type() override { return Type::Case; }

    virtual Value *to_ruby(Env *) override;

    void add_when_node(Node *node) {
        m_when_nodes.push(node);
    }

    void set_else_node(BlockNode *node) {
        m_else_node = node;
    }

private:
    Node *m_subject { nullptr };
    Vector<Node *> m_when_nodes {};
    BlockNode *m_else_node { nullptr };
};

struct CaseWhenNode : Node {
    CaseWhenNode(Token token, Node *condition, BlockNode *body)
        : Node { token }
        , m_condition { condition }
        , m_body { body } {
        assert(m_condition);
        assert(m_body);
    }

    virtual Type type() override { return Type::CaseWhen; }

    virtual Value *to_ruby(Env *) override;

private:
    Node *m_condition { nullptr };
    BlockNode *m_body { nullptr };
};

struct AttrAssignNode : CallNode {
    AttrAssignNode(Token token, Node *receiver, const char *message)
        : CallNode { token, receiver, message } { }

    AttrAssignNode(Token token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() override { return Type::AttrAssign; }

    virtual Value *to_ruby(Env *) override;
};

struct SafeCallNode : CallNode {
    SafeCallNode(Token token, Node *receiver, const char *message)
        : CallNode { token, receiver, message } { }

    SafeCallNode(Token token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() override { return Type::SafeCall; }

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

struct Colon2Node : Node {
    Colon2Node(Token token, Node *left, const char *name)
        : Node { token }
        , m_left { left }
        , m_name { name } {
        assert(m_left);
        assert(m_name);
    }

    virtual Type type() override { return Type::Colon2; }

    virtual Value *to_ruby(Env *) override;

private:
    Token m_token {};
    Node *m_left { nullptr };
    const char *m_name { nullptr };
};

struct Colon3Node : Node {
    Colon3Node(Token token, const char *name)
        : Node { token }
        , m_name { name } {
        assert(m_name);
    }

    virtual Type type() override { return Type::Colon3; }

    virtual Value *to_ruby(Env *) override;

private:
    Token m_token {};
    const char *m_name { nullptr };
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
    DefNode(Token token, Node *self_node, IdentifierNode *name, Vector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_self_node { self_node }
        , m_name { name }
        , m_args { args }
        , m_body { body } { }

    DefNode(Token token, IdentifierNode *name, Vector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_args { args }
        , m_body { body } { }

    virtual Type type() override { return Type::Def; }

    virtual Value *to_ruby(Env *) override;

private:
    SexpValue *build_args_sexp(Env *);

    Node *m_self_node { nullptr };
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

    void append_to_name(char c) {
        m_token.set_literal(std::string(m_token.literal()) + c);
    }

    virtual bool is_callable() override {
        switch (token_type()) {
        case Token::Type::BareName:
        case Token::Type::Constant:
            return !m_is_lvar;
        default:
            return false;
        }
    }

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

struct InterpolatedShellNode : Node {
    InterpolatedShellNode(Token token)
        : Node { token } { }

    virtual Type type() override { return Type::InterpolatedShell; }

    virtual Value *to_ruby(Env *) override;

    void add_node(Node *node) { m_nodes.push(node); };

private:
    Vector<Node *> m_nodes {};
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

struct NotNode : Node {
    NotNode(Token token, Node *expression)
        : Node { token }
        , m_expression { expression } {
        assert(m_expression);
    }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::Not; }

private:
    Node *m_expression { nullptr };
};

struct NilSexpNode : Node {
    NilSexpNode(Token token)
        : Node { token } { }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::NilSexp; }
};

struct OpAssignAndNode : Node {
    OpAssignAndNode(Token token, IdentifierNode *name, Node *value)
        : Node { token }
        , m_name { name }
        , m_value { value } {
        assert(m_name);
        assert(m_value);
    }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssignAnd; }

private:
    IdentifierNode *m_name { nullptr };
    Node *m_value { nullptr };
};

struct OpAssignOrNode : Node {
    OpAssignOrNode(Token token, IdentifierNode *name, Node *value)
        : Node { token }
        , m_name { name }
        , m_value { value } {
        assert(m_name);
        assert(m_value);
    }

    virtual Value *to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssignOr; }

private:
    IdentifierNode *m_name { nullptr };
    Node *m_value { nullptr };
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

struct RegexpNode : Node {
    RegexpNode(Token token, Value *value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Regexp; }

    virtual Value *to_ruby(Env *) override;

    Value *value() { return m_value; }

private:
    Value *m_value { nullptr };
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

struct SelfNode : Node {
    SelfNode(Token token)
        : Node { token } { }

    virtual Type type() override { return Type::Self; }

    virtual Value *to_ruby(Env *) override;
};

struct ShellNode : Node {
    ShellNode(Token token, Value *value)
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
