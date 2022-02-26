#pragma once

#include "natalie_parser/token.hpp"
#include "tm/hashmap.hpp"
#include "tm/shared_ptr.hpp"
#include "tm/string.hpp"
#include "tm/vector.hpp"

namespace NatalieParser {

using namespace TM;

class Node {
public:
    enum class Type {
        Alias,
        Arg,
        Array,
        ArrayPattern,
        Assignment,
        Begin,
        BeginRescue,
        Block,
        BlockPass,
        Break,
        Call,
        Case,
        CaseIn,
        CaseWhen,
        Class,
        Colon2,
        Colon3,
        Constant,
        Def,
        Defined,
        EvaluateToString,
        False,
        Float,
        Hash,
        HashPattern,
        Identifier,
        If,
        Integer,
        Iter,
        InterpolatedRegexp,
        InterpolatedShell,
        InterpolatedString,
        KeywordArg,
        KeywordSplat,
        LogicalAnd,
        LogicalOr,
        Match,
        Module,
        MultipleAssignment,
        Next,
        Nil,
        NilSexp,
        Not,
        OpAssign,
        OpAssignAccessor,
        OpAssignAnd,
        OpAssignOr,
        Pin,
        Range,
        Regexp,
        Return,
        SafeCall,
        Sclass,
        Self,
        Shell,
        Splat,
        SplatValue,
        StabbyProc,
        String,
        Super,
        Symbol,
        ToArray,
        True,
        Until,
        While,
        Yield,
    };

    Node(const Token &token)
        : m_token { token } { }

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    virtual ~Node() { }

    virtual Type type() = 0;

    virtual bool is_callable() const { return false; }

    virtual Node *clone() const {
        printf("Need to implement Node::clone() in a subclass...\n");
        TM_UNREACHABLE();
    }

    SharedPtr<String> file() const { return m_token.file(); }
    size_t line() const { return m_token.line(); }
    size_t column() const { return m_token.column(); }

    const Token &token() const { return m_token; }

protected:
    Token m_token {};
};

class NodeWithArgs : public Node {
public:
    NodeWithArgs(const Token &token)
        : Node { token } { }

    NodeWithArgs(const Token &token, Vector<Node *> &args)
        : Node { token } {
        for (auto arg : args)
            add_arg(arg);
    }

    ~NodeWithArgs() {
        for (auto arg : m_args)
            delete arg;
    }

    void add_arg(Node *arg) {
        m_args.push(arg);
    }

    Vector<Node *> &args() { return m_args; }

protected:
    Vector<Node *> m_args {};
};

class SymbolNode;

class AliasNode : public Node {
public:
    AliasNode(const Token &token, SymbolNode *new_name, SymbolNode *existing_name)
        : Node { token }
        , m_new_name { new_name }
        , m_existing_name { existing_name } { }

    ~AliasNode();

    virtual Type type() override { return Type::Alias; }

    SymbolNode *new_name() const { return m_new_name; }
    SymbolNode *existing_name() const { return m_existing_name; }

private:
    SymbolNode *m_new_name { nullptr };
    SymbolNode *m_existing_name { nullptr };
};

class ArgNode : public Node {
public:
    ArgNode(const Token &token)
        : Node { token } { }

    ArgNode(const Token &token, SharedPtr<String> name)
        : Node { token }
        , m_name { name } { }

    ArgNode(const ArgNode &other)
        : Node { other.m_token }
        , m_name { other.m_name }
        , m_block_arg { other.m_block_arg }
        , m_splat { other.m_splat }
        , m_kwsplat { other.m_kwsplat }
        , m_value { other.m_value } { }

    ~ArgNode() {
        delete m_value;
    }

    virtual Node *clone() const override {
        return new ArgNode(*this);
    }

    virtual Type type() override { return Type::Arg; }

    SharedPtr<String> name() const { return m_name; }

    bool splat() const { return m_splat; }
    void set_splat(bool splat) { m_splat = splat; }

    bool kwsplat() const { return m_kwsplat; }
    void set_kwsplat(bool kwsplat) { m_kwsplat = kwsplat; }

    bool block_arg() const { return m_block_arg; }
    void set_block_arg(bool block_arg) { m_block_arg = block_arg; }

    Node *value() const { return m_value; }
    void set_value(Node *value) { m_value = value; }

    void add_to_locals(TM::Hashmap<const char *> &locals) {
        locals.set(m_name->c_str());
    }

protected:
    SharedPtr<String> m_name {};
    bool m_block_arg { false };
    bool m_splat { false };
    bool m_kwsplat { false };
    Node *m_value { nullptr };
};

class ArrayNode : public Node {
public:
    ArrayNode(const Token &token)
        : Node { token } { }

    ArrayNode(const ArrayNode &other)
        : Node { other.m_token } {
        for (auto node : other.m_nodes)
            m_nodes.push(node);
    }

    ~ArrayNode() {
        for (auto node : m_nodes)
            delete node;
    }

    virtual Node *clone() const override {
        return new ArrayNode(*this);
    }

    virtual Type type() override { return Type::Array; }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    Vector<Node *> &nodes() { return m_nodes; }

protected:
    Vector<Node *> m_nodes {};
};

class ArrayPatternNode : public ArrayNode {
public:
    ArrayPatternNode(const Token &token)
        : ArrayNode { token } { }

    virtual Type type() override { return Type::ArrayPattern; }
};

class BlockPassNode : public Node {
public:
    BlockPassNode(const Token &token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    ~BlockPassNode() {
        delete m_node;
    }

    virtual Type type() override { return Type::BlockPass; }

    Node *node() const { return m_node; }

protected:
    Node *m_node { nullptr };
};

class BreakNode : public NodeWithArgs {
public:
    BreakNode(const Token &token, Node *arg = nullptr)
        : NodeWithArgs { token }
        , m_arg { arg } { }

    ~BreakNode() {
        delete m_arg;
    }

    virtual Type type() override { return Type::Break; }

    Node *arg() const { return m_arg; }

protected:
    Node *m_arg { nullptr };
};

class IdentifierNode;

class AssignmentNode : public Node {
public:
    AssignmentNode(const Token &token, Node *identifier, Node *value)
        : Node { token }
        , m_identifier { identifier }
        , m_value { value } {
        assert(m_identifier);
        assert(m_value);
    }

    ~AssignmentNode() {
        delete m_identifier;
        delete m_value;
    }

    virtual Type type() override { return Type::Assignment; }

    Node *identifier() const { return m_identifier; }
    Node *value() const { return m_value; }

protected:
    Node *m_identifier { nullptr };
    Node *m_value { nullptr };
};

class BlockNode;
class BeginRescueNode;

class BeginNode : public Node {
public:
    BeginNode(const Token &token, BlockNode *body)
        : Node { token }
        , m_body { body } {
        assert(m_body);
    }

    ~BeginNode();

    virtual Type type() override { return Type::Begin; }

    void add_rescue_node(BeginRescueNode *node) { m_rescue_nodes.push(node); }
    bool no_rescue_nodes() const { return m_rescue_nodes.size() == 0; }

    bool has_ensure_body() const { return m_ensure_body ? true : false; }
    void set_else_body(BlockNode *else_body) { m_else_body = else_body; }
    void set_ensure_body(BlockNode *ensure_body) { m_ensure_body = ensure_body; }

    BlockNode *body() const { return m_body; }
    BlockNode *else_body() const { return m_else_body; }
    BlockNode *ensure_body() const { return m_ensure_body; }

    Vector<BeginRescueNode *> &rescue_nodes() { return m_rescue_nodes; }

protected:
    BlockNode *m_body { nullptr };
    BlockNode *m_else_body { nullptr };
    BlockNode *m_ensure_body { nullptr };
    Vector<BeginRescueNode *> m_rescue_nodes {};
};

class BeginRescueNode : public Node {
public:
    BeginRescueNode(const Token &token)
        : Node { token } { }

    ~BeginRescueNode();

    virtual Type type() override { return Type::BeginRescue; }

    void add_exception_node(Node *node) {
        m_exceptions.push(node);
    }

    void set_exception_name(IdentifierNode *name) {
        m_name = name;
    }

    void set_body(BlockNode *body) { m_body = body; }

    Node *name_to_node();

    IdentifierNode *name() const { return m_name; }
    Vector<Node *> &exceptions() { return m_exceptions; }
    BlockNode *body() const { return m_body; }

protected:
    IdentifierNode *m_name { nullptr };
    Vector<Node *> m_exceptions {};
    BlockNode *m_body { nullptr };
};

class BlockNode : public Node {
public:
    BlockNode(const Token &token)
        : Node { token } { }

    BlockNode(const Token &token, Node *single_node)
        : Node { token } {
        add_node(single_node);
    }

    ~BlockNode() {
        for (auto node : m_nodes)
            delete node;
    }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    virtual Type type() override { return Type::Block; }

    Vector<Node *> &nodes() { return m_nodes; }
    bool is_empty() const { return m_nodes.is_empty(); }

    bool has_one_node() const { return m_nodes.size() == 1; }

    Node *without_unnecessary_nesting() {
        if (has_one_node())
            return m_nodes[0];
        else
            return this;
    }

protected:
    Vector<Node *> m_nodes {};
};

class CallNode : public NodeWithArgs {
public:
    CallNode(const Token &token, Node *receiver, SharedPtr<String> message)
        : NodeWithArgs { token }
        , m_receiver { receiver }
        , m_message { message } {
        assert(m_receiver);
        assert(m_message);
    }

    CallNode(const Token &token, CallNode &node)
        : NodeWithArgs { token }
        , m_receiver { node.m_receiver }
        , m_message { node.m_message } {
        for (auto arg : node.m_args) {
            add_arg(arg);
        }
    }

    ~CallNode() {
        delete m_receiver;
    }

    virtual Type type() override { return Type::Call; }

    virtual bool is_callable() const override { return true; }

    Node *receiver() const { return m_receiver; }

    SharedPtr<String> message() const { return m_message; }

    void set_message(SharedPtr<String> message) {
        assert(message);
        m_message = message;
    }

    void set_message(const char *message) {
        assert(message);
        m_message = new String(message);
    }

protected:
    Node *m_receiver { nullptr };
    SharedPtr<String> m_message {};
};

class CaseNode : public Node {
public:
    CaseNode(const Token &token, Node *subject)
        : Node { token }
        , m_subject { subject } {
        assert(m_subject);
    }

    ~CaseNode() {
        delete m_subject;
        for (auto node : m_nodes)
            delete node;
        delete m_else_node;
    }

    virtual Type type() override { return Type::Case; }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    void set_else_node(BlockNode *node) {
        m_else_node = node;
    }

    Node *subject() const { return m_subject; }
    Vector<Node *> &nodes() { return m_nodes; }
    BlockNode *else_node() const { return m_else_node; }

protected:
    Node *m_subject { nullptr };
    Vector<Node *> m_nodes {};
    BlockNode *m_else_node { nullptr };
};

class CaseInNode : public Node {
public:
    CaseInNode(const Token &token, Node *pattern, BlockNode *body)
        : Node { token }
        , m_pattern { pattern }
        , m_body { body } {
        assert(m_pattern);
        assert(m_body);
    }

    ~CaseInNode() {
        delete m_pattern;
        delete m_body;
    }

    virtual Type type() override { return Type::CaseIn; }

    Node *pattern() const { return m_pattern; }
    BlockNode *body() const { return m_body; }

protected:
    Node *m_pattern { nullptr };
    BlockNode *m_body { nullptr };
};

class CaseWhenNode : public Node {
public:
    CaseWhenNode(const Token &token, Node *condition, BlockNode *body)
        : Node { token }
        , m_condition { condition }
        , m_body { body } {
        assert(m_condition);
        assert(m_body);
    }

    ~CaseWhenNode() {
        delete m_condition;
        delete m_body;
    }

    virtual Type type() override { return Type::CaseWhen; }

    Node *condition() const { return m_condition; }
    BlockNode *body() const { return m_body; }

protected:
    Node *m_condition { nullptr };
    BlockNode *m_body { nullptr };
};

class SafeCallNode : public CallNode {
public:
    SafeCallNode(const Token &token, Node *receiver, SharedPtr<String> message)
        : CallNode { token, receiver, message } { }

    SafeCallNode(const Token &token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() override { return Type::SafeCall; }
};

class ClassNode : public Node {
public:
    ClassNode(const Token &token, Node *name, Node *superclass, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_superclass { superclass }
        , m_body { body } { }

    ~ClassNode();

    virtual Type type() override { return Type::Class; }

    Node *name() const { return m_name; }
    Node *superclass() const { return m_superclass; }
    BlockNode *body() const { return m_body; }

protected:
    Node *m_name { nullptr };
    Node *m_superclass { nullptr };
    BlockNode *m_body { nullptr };
};

class Colon2Node : public Node {
public:
    Colon2Node(const Token &token, Node *left, SharedPtr<String> name)
        : Node { token }
        , m_left { left }
        , m_name { name } {
        assert(m_left);
        assert(m_name);
    }

    ~Colon2Node() {
        delete m_left;
    }

    virtual Type type() override { return Type::Colon2; }

    Node *left() const { return m_left; }
    SharedPtr<String> name() const { return m_name; }

protected:
    Node *m_left { nullptr };
    SharedPtr<String> m_name {};
};

class Colon3Node : public Node {
public:
    Colon3Node(const Token &token, SharedPtr<String> name)
        : Node { token }
        , m_name { name } { }

    virtual Type type() override { return Type::Colon3; }

    SharedPtr<String> name() const { return m_name; }

protected:
    SharedPtr<String> m_name {};
};

class ConstantNode : public Node {
public:
    ConstantNode(const Token &token)
        : Node { token } { }

    virtual Type type() override { return Type::Constant; }

    SharedPtr<String> name() const { return m_token.literal_string(); }
};

class IntegerNode : public Node {
public:
    IntegerNode(const Token &token, long long number)
        : Node { token }
        , m_number { number } { }

    virtual Type type() override { return Type::Integer; }

    long long number() const { return m_number; }

protected:
    long long m_number;
};

class FloatNode : public Node {
public:
    FloatNode(const Token &token, double number)
        : Node { token }
        , m_number { number } { }

    virtual Type type() override { return Type::Float; }

    double number() const { return m_number; }

protected:
    double m_number;
};

class DefinedNode : public Node {
public:
    DefinedNode(const Token &token, Node *arg)
        : Node { token }
        , m_arg { arg } {
        assert(arg);
    }

    ~DefinedNode() {
        delete m_arg;
    }

    virtual Type type() override { return Type::Defined; }

    Node *arg() const { return m_arg; }

protected:
    Node *m_arg { nullptr };
};

class DefNode : public NodeWithArgs {
public:
    DefNode(const Token &token, Node *self_node, SharedPtr<String> name, Vector<Node *> &args, BlockNode *body)
        : NodeWithArgs { token, args }
        , m_self_node { self_node }
        , m_name { name }
        , m_body { body } { }

    DefNode(const Token &token, SharedPtr<String> name, Vector<Node *> &args, BlockNode *body)
        : NodeWithArgs { token, args }
        , m_name { name }
        , m_body { body } { }

    ~DefNode() {
        delete m_self_node;
        delete m_body;
    }

    virtual Type type() override { return Type::Def; }

    Node *self_node() const { return m_self_node; }
    SharedPtr<String> name() const { return m_name; }
    BlockNode *body() const { return m_body; }

protected:
    Node *m_self_node { nullptr };
    SharedPtr<String> m_name {};
    BlockNode *m_body { nullptr };
};

class EvaluateToStringNode : public Node {
public:
    EvaluateToStringNode(const Token &token, Node *node)
        : Node { token }
        , m_node { node } { }

    ~EvaluateToStringNode() {
        delete m_node;
    }

    virtual Type type() override { return Type::EvaluateToString; }

    Node *node() const { return m_node; }

protected:
    Node *m_node { nullptr };
};

class FalseNode : public Node {
public:
    FalseNode(const Token &token)
        : Node { token } { }

    virtual Type type() override { return Type::False; }
};

class HashNode : public Node {
public:
    HashNode(const Token &token)
        : Node { token } { }

    ~HashNode() {
        for (auto node : m_nodes)
            delete node;
    }

    virtual Type type() override { return Type::Hash; }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    Vector<Node *> &nodes() { return m_nodes; }

protected:
    Vector<Node *> m_nodes {};
};

class HashPatternNode : public HashNode {
public:
    HashPatternNode(const Token &token)
        : HashNode { token } { }

    virtual Type type() override { return Type::HashPattern; }
};

class IdentifierNode : public Node {
public:
    IdentifierNode(const Token &token, bool is_lvar)
        : Node { token }
        , m_is_lvar { is_lvar } { }

    virtual Type type() override { return Type::Identifier; }

    Token::Type token_type() const { return m_token.type(); }

    SharedPtr<String> name() const { return m_token.literal_string(); }

    void append_to_name(char c) {
        auto literal = m_token.literal_string();
        literal->append_char(c);
        m_token.set_literal(literal);
    }

    virtual bool is_callable() const override {
        switch (token_type()) {
        case Token::Type::BareName:
        case Token::Type::Constant:
            return !m_is_lvar;
        default:
            return false;
        }
    }

    bool is_lvar() const { return m_is_lvar; }
    void set_is_lvar(bool is_lvar) { m_is_lvar = is_lvar; }

    size_t nth_ref() const {
        auto str = name();
        size_t len = str->length();
        if (*str == "$0")
            return 0;
        if (len <= 1)
            return 0;
        int ref = 0;
        for (size_t i = 1; i < len; i++) {
            char c = (*str)[i];
            if (i == 1 && c == '0')
                return 0;
            int num = c - 48;
            if (num < 0 || num > 9)
                return 0;
            ref *= 10;
            ref += num;
        }
        return ref;
    }

    void add_to_locals(TM::Hashmap<const char *> &locals) const {
        if (token_type() == Token::Type::BareName)
            locals.set(name()->c_str());
    }

protected:
    bool m_is_lvar { false };
};

class IfNode : public Node {
public:
    IfNode(const Token &token, Node *condition, Node *true_expr, Node *false_expr)
        : Node { token }
        , m_condition { condition }
        , m_true_expr { true_expr }
        , m_false_expr { false_expr } {
        assert(m_condition);
        assert(m_true_expr);
        assert(m_false_expr);
    }

    ~IfNode() {
        delete m_condition;
        delete m_true_expr;
        delete m_false_expr;
    }

    virtual Type type() override { return Type::If; }

    Node *condition() const { return m_condition; }
    Node *true_expr() const { return m_true_expr; }
    Node *false_expr() const { return m_false_expr; }

protected:
    Node *m_condition { nullptr };
    Node *m_true_expr { nullptr };
    Node *m_false_expr { nullptr };
};

class IterNode : public NodeWithArgs {
public:
    IterNode(const Token &token, Node *call, Vector<Node *> &args, BlockNode *body)
        : NodeWithArgs { token, args }
        , m_call { call }
        , m_body { body } {
        assert(m_call);
        assert(m_body);
    }

    ~IterNode() {
        delete m_call;
        delete m_body;
    }

    virtual Type type() override { return Type::Iter; }

    Node *call() const { return m_call; }
    BlockNode *body() const { return m_body; }

protected:
    Node *m_call { nullptr };
    BlockNode *m_body { nullptr };
};

class InterpolatedNode : public Node {
public:
    InterpolatedNode(const Token &token)
        : Node { token } { }

    ~InterpolatedNode() {
        for (auto node : m_nodes)
            delete node;
    }

    void add_node(Node *node) { m_nodes.push(node); };

    Vector<Node *> &nodes() { return m_nodes; }

protected:
    Vector<Node *> m_nodes {};
};

class InterpolatedRegexpNode : public InterpolatedNode {
public:
    InterpolatedRegexpNode(const Token &token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedRegexp; }

    int options() const { return m_options; }
    void set_options(int options) { m_options = options; }

private:
    int m_options { 0 };
};

class InterpolatedShellNode : public InterpolatedNode {
public:
    InterpolatedShellNode(const Token &token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedShell; }
};

class InterpolatedStringNode : public InterpolatedNode {
public:
    InterpolatedStringNode(const Token &token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedString; }
};

class KeywordArgNode : public ArgNode {
public:
    KeywordArgNode(const Token &token, SharedPtr<String> name)
        : ArgNode { token, name } { }

    virtual Type type() override { return Type::KeywordArg; }
};

class KeywordSplatNode : public Node {
public:
    KeywordSplatNode(const Token &token)
        : Node { token } { }

    KeywordSplatNode(const Token &token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    ~KeywordSplatNode() {
        delete m_node;
    }

    virtual Type type() override { return Type::KeywordSplat; }

    Node *node() const { return m_node; }

protected:
    Node *m_node { nullptr };
};

class LogicalAndNode : public Node {
public:
    LogicalAndNode(const Token &token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    ~LogicalAndNode() {
        delete m_left;
        delete m_right;
    }

    virtual Type type() override { return Type::LogicalAnd; }

    Node *left() const { return m_left; }
    Node *right() const { return m_right; }

protected:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};

class LogicalOrNode : public Node {
public:
    LogicalOrNode(const Token &token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    ~LogicalOrNode() {
        delete m_left;
        delete m_right;
    }

    virtual Type type() override { return Type::LogicalOr; }

    Node *left() const { return m_left; }
    Node *right() const { return m_right; }

protected:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};

class RegexpNode;

class MatchNode : public Node {
public:
    MatchNode(const Token &token, RegexpNode *regexp, Node *arg, bool regexp_on_left)
        : Node { token }
        , m_regexp { regexp }
        , m_arg { arg }
        , m_regexp_on_left { regexp_on_left } { }

    ~MatchNode();

    virtual Type type() override { return Type::Match; }

    RegexpNode *regexp() const { return m_regexp; }
    Node *arg() const { return m_arg; }
    bool regexp_on_left() const { return m_regexp_on_left; }

protected:
    RegexpNode *m_regexp { nullptr };
    Node *m_arg { nullptr };
    bool m_regexp_on_left { false };
};

class ModuleNode : public Node {
public:
    ModuleNode(const Token &token, Node *name, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_body { body } { }

    ~ModuleNode() {
        delete m_name;
        delete m_body;
    }

    virtual Type type() override { return Type::Module; }

    Node *name() const { return m_name; }
    BlockNode *body() const { return m_body; }

protected:
    Node *m_name { nullptr };
    BlockNode *m_body { nullptr };
};

class MultipleAssignmentNode : public ArrayNode {
public:
    MultipleAssignmentNode(const Token &token)
        : ArrayNode { token } { }

    virtual Type type() override { return Type::MultipleAssignment; }

    void add_locals(TM::Hashmap<const char *> &);
};

class NextNode : public Node {
public:
    NextNode(const Token &token, Node *arg = nullptr)
        : Node { token }
        , m_arg { arg } { }

    ~NextNode() {
        delete m_arg;
    }

    virtual Type type() override { return Type::Next; }

    Node *arg() const { return m_arg; }

protected:
    Node *m_arg { nullptr };
};

class NilNode : public Node {
public:
    NilNode(const Token &token)
        : Node { token } { }

    virtual Type type() override { return Type::Nil; }
};

class NotNode : public Node {
public:
    NotNode(const Token &token, Node *expression)
        : Node { token }
        , m_expression { expression } {
        assert(m_expression);
    }

    ~NotNode() {
        delete m_expression;
    }

    virtual Type type() override { return Type::Not; }

    Node *expression() const { return m_expression; }

protected:
    Node *m_expression { nullptr };
};

class NilSexpNode : public Node {
public:
    NilSexpNode(const Token &token)
        : Node { token } { }

    virtual Type type() override { return Type::NilSexp; }
};

class OpAssignNode : public Node {
public:
    OpAssignNode(const Token &token, IdentifierNode *name, Node *value)
        : Node { token }
        , m_name { name }
        , m_value { value } {
        assert(m_name);
        assert(m_value);
    }

    OpAssignNode(const Token &token, SharedPtr<String> op, IdentifierNode *name, Node *value)
        : Node { token }
        , m_op { op }
        , m_name { name }
        , m_value { value } {
        assert(m_op);
        assert(m_name);
        assert(m_value);
    }

    ~OpAssignNode() {
        delete m_name;
        delete m_value;
    }

    virtual Type type() override { return Type::OpAssign; }

    SharedPtr<String> op() const { return m_op; }
    IdentifierNode *name() const { return m_name; }
    Node *value() const { return m_value; }

protected:
    SharedPtr<String> m_op {};
    IdentifierNode *m_name { nullptr };
    Node *m_value { nullptr };
};

class OpAssignAccessorNode : public NodeWithArgs {
public:
    OpAssignAccessorNode(const Token &token, SharedPtr<String> op, Node *receiver, SharedPtr<String> message, Node *value, Vector<Node *> &args)
        : NodeWithArgs { token, args }
        , m_op { op }
        , m_receiver { receiver }
        , m_message { message }
        , m_value { value } {
        assert(m_op);
        assert(m_receiver);
        assert(m_message);
        assert(m_value);
    }

    ~OpAssignAccessorNode() {
        delete m_receiver;
        delete m_value;
    }

    virtual Type type() override { return Type::OpAssignAccessor; }

    SharedPtr<String> op() const { return m_op; }
    Node *receiver() const { return m_receiver; }
    SharedPtr<String> message() const { return m_message; }
    Node *value() const { return m_value; }

protected:
    SharedPtr<String> m_op {};
    Node *m_receiver { nullptr };
    SharedPtr<String> m_message {};
    Node *m_value { nullptr };
};

class OpAssignAndNode : public OpAssignNode {
public:
    OpAssignAndNode(const Token &token, IdentifierNode *name, Node *value)
        : OpAssignNode { token, name, value } { }

    virtual Type type() override { return Type::OpAssignAnd; }
};

class OpAssignOrNode : public OpAssignNode {
public:
    OpAssignOrNode(const Token &token, IdentifierNode *name, Node *value)
        : OpAssignNode { token, name, value } { }

    virtual Type type() override { return Type::OpAssignOr; }
};

class PinNode : public Node {
public:
    PinNode(const Token &token, Node *identifier)
        : Node { token }
        , m_identifier { identifier } {
        assert(m_identifier);
    }

    ~PinNode() {
        delete m_identifier;
    }

    virtual Type type() override { return Type::Pin; }

    Node *identifier() const { return m_identifier; }

protected:
    Node *m_identifier { nullptr };
};

class RangeNode : public Node {
public:
    RangeNode(const Token &token, Node *first, Node *last, bool exclude_end)
        : Node { token }
        , m_first { first }
        , m_last { last }
        , m_exclude_end { exclude_end } {
        assert(m_first);
        assert(m_last);
    }

    ~RangeNode() {
        delete m_first;
        delete m_last;
    }

    virtual Type type() override { return Type::Range; }

    Node *first() const { return m_first; }
    Node *last() const { return m_last; }
    bool exclude_end() const { return m_exclude_end; }

protected:
    Node *m_first { nullptr };
    Node *m_last { nullptr };
    bool m_exclude_end { false };
};

class RegexpNode : public Node {
public:
    RegexpNode(const Token &token, SharedPtr<String> pattern)
        : Node { token }
        , m_pattern { pattern } {
        assert(m_pattern);
    }

    virtual Type type() override { return Type::Regexp; }

    SharedPtr<String> pattern() const { return m_pattern; }
    SharedPtr<String> options() const { return m_options; }

    void set_options(SharedPtr<String> options) { m_options = options; }

protected:
    SharedPtr<String> m_pattern {};
    SharedPtr<String> m_options {};
};

class ReturnNode : public Node {
public:
    ReturnNode(const Token &token, Node *value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    ~ReturnNode() {
        delete m_value;
    }

    virtual Type type() override { return Type::Return; }

    Node *value() const { return m_value; }

protected:
    Node *m_value { nullptr };
};

class SclassNode : public Node {
public:
    SclassNode(const Token &token, Node *klass, BlockNode *body)
        : Node { token }
        , m_klass { klass }
        , m_body { body } { }

    ~SclassNode() {
        delete m_klass;
        delete m_body;
    }

    virtual Type type() override { return Type::Sclass; }

    Node *klass() const { return m_klass; }
    BlockNode *body() const { return m_body; }

protected:
    Node *m_klass { nullptr };
    BlockNode *m_body { nullptr };
};

class SelfNode : public Node {
public:
    SelfNode(const Token &token)
        : Node { token } { }

    virtual Type type() override { return Type::Self; }
};

class ShellNode : public Node {
public:
    ShellNode(const Token &token, SharedPtr<String> string)
        : Node { token }
        , m_string { string } {
        assert(m_string);
    }

    virtual Type type() override { return Type::Shell; }

    SharedPtr<String> string() const { return m_string; }

protected:
    SharedPtr<String> m_string {};
};

class SplatNode : public Node {
public:
    SplatNode(const Token &token)
        : Node { token } { }

    SplatNode(const Token &token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    ~SplatNode() {
        delete m_node;
    }

    virtual Type type() override { return Type::Splat; }

    Node *node() const { return m_node; }

protected:
    Node *m_node { nullptr };
};

class SplatValueNode : public Node {
public:
    SplatValueNode(const Token &token, Node *value)
        : Node { token }
        , m_value { value } { }

    ~SplatValueNode() {
        delete m_value;
    }

    virtual Type type() override { return Type::SplatValue; }

    Node *value() const { return m_value; }

protected:
    Node *m_value { nullptr };
};

class StabbyProcNode : public NodeWithArgs {
public:
    using NodeWithArgs::NodeWithArgs;

    virtual Type type() override { return Type::StabbyProc; }
};

class StringNode : public Node {
public:
    StringNode(const Token &token, SharedPtr<String> string)
        : Node { token }
        , m_string { string } {
        assert(m_string);
    }

    virtual Type type() override { return Type::String; }

    SharedPtr<String> string() const { return m_string; }

protected:
    SharedPtr<String> m_string {};
};

class SymbolNode : public Node {
public:
    SymbolNode(const Token &token, SharedPtr<String> name)
        : Node { token }
        , m_name { name } { }

    virtual Type type() override { return Type::Symbol; }

    SharedPtr<String> name() const { return m_name; }

protected:
    SharedPtr<String> m_name {};
};

class ToArrayNode : public Node {
public:
    ToArrayNode(const Token &token, Node *value)
        : Node { token }
        , m_value { value } { }

    ~ToArrayNode() {
        delete m_value;
    }

    virtual Type type() override { return Type::ToArray; }

    Node *value() const { return m_value; }

protected:
    Node *m_value { nullptr };
};

class TrueNode : public Node {
public:
    TrueNode(const Token &token)
        : Node { token } { }

    virtual Type type() override { return Type::True; }
};

class SuperNode : public NodeWithArgs {
public:
    SuperNode(const Token &token)
        : NodeWithArgs { token } { }

    virtual Type type() override { return Type::Super; }

    bool parens() const { return m_parens; }
    void set_parens(bool parens) { m_parens = parens; }

    bool empty_parens() const { return m_parens && m_args.is_empty(); }

protected:
    bool m_parens { false };
};

class WhileNode : public Node {
public:
    WhileNode(const Token &token, Node *condition, BlockNode *body, bool pre)
        : Node { token }
        , m_condition { condition }
        , m_body { body }
        , m_pre { pre } {
        assert(m_condition);
        assert(m_body);
    }

    ~WhileNode() {
        delete m_condition;
        delete m_body;
    }

    virtual Type type() override { return Type::While; }

    Node *condition() const { return m_condition; }
    BlockNode *body() const { return m_body; }
    bool pre() const { return m_pre; }

protected:
    Node *m_condition { nullptr };
    BlockNode *m_body { nullptr };
    bool m_pre { false };
};

class UntilNode : public WhileNode {
public:
    UntilNode(const Token &token, Node *condition, BlockNode *body, bool pre)
        : WhileNode { token, condition, body, pre } { }

    virtual Type type() override { return Type::Until; }
};

class YieldNode : public NodeWithArgs {
public:
    YieldNode(const Token &token)
        : NodeWithArgs { token } { }

    virtual Type type() override { return Type::Yield; }
};
}
