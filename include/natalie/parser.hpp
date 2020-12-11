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
            return env->nil_obj();
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

        virtual Value *to_ruby(Env *env) override {
            auto *array = new SexpValue { env, { SymbolValue::intern(env, "block") } };
            for (auto node : m_nodes) {
                array->push(node->to_ruby(env));
            }
            return array;
        }

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

        virtual Value *to_ruby(Env *env) override {
            auto sexp = new SexpValue { env, {
                                                 SymbolValue::intern(env, "call"),
                                                 m_receiver->to_ruby(env),
                                                 m_message,
                                             } };

            for (auto arg : m_args) {
                sexp->push(arg->to_ruby(env));
            }
            return sexp;
        }

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

        virtual Value *to_ruby(Env *env) override {
            auto sexp = new SexpValue { env, {
                                                 SymbolValue::intern(env, "defn"),
                                                 SymbolValue::intern(env, m_name->name()),
                                                 build_args_sexp(env),
                                             } };
            if (m_body->is_empty()) {
                sexp->push(new SexpValue { env, { SymbolValue::intern(env, "nil") } });
            } else {
                for (auto node : *(m_body->nodes())) {
                    sexp->push(node->to_ruby(env));
                }
            }
            return sexp;
        }

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

        virtual Value *to_ruby(Env *env) override {
            if (m_is_lvar) {
                return new SexpValue { env, { SymbolValue::intern(env, "lvar"), SymbolValue::intern(env, name()) } };
            } else {
                return new SexpValue { env, { SymbolValue::intern(env, "call"), env->nil_obj(), SymbolValue::intern(env, name()) } };
            }
        }

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

        virtual Type type() override { return Type::Symbol; }

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

        virtual Type type() override { return Type::String; }

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
        default:
            return LOWEST;
        }
    }

    Node *parse_expression(Env *env, Precedence precedence, Vector<SymbolValue *> *locals) {
#ifdef NAT_DEBUG_PARSER
        printf("entering parse_expression with precedence = %d, current token = %s\n", precedence, current_token().to_ruby(env)->inspect_str(env));
#endif
        skip_newlines();

        auto null_fn = null_denotation(current_token().type());
        if (!null_fn) {
            raise_unexpected(env, "null_fn");
        }

        Node *left = (this->*null_fn)(env, locals);

        while (current_token().is_valid() && precedence < get_precedence()) {
#ifdef NAT_DEBUG_PARSER
            printf("while loop: current token = %s\n", current_token().to_ruby(env)->inspect_str(env));
#endif
            auto left_fn = left_denotation(current_token().type());
            if (!left_fn) {
                raise_unexpected(env, "left_fn");
            }

            left = (this->*left_fn)(env, left, locals);
        }
        return left;
    }

    Node *tree(Env *env) {
        auto tree = new BlockNode {};
        current_token().validate(env);
        auto locals = new Vector<SymbolValue *> {};
        while (!current_token().is_eof()) {
            auto exp = parse_expression(env, LOWEST, locals);
            tree->add_node(exp);
            current_token().validate(env);
            next_expression(env);
        }
        return tree;
    }

private:
    Node *parse_def(Env *env, Vector<SymbolValue *> *) {
        advance();
        auto locals = new Vector<SymbolValue *> {};
        auto name = static_cast<IdentifierNode *>(parse_identifier(env, locals));
        auto args = new Vector<Node *> {};
        if (current_token().is_lparen()) {
            advance();
            if (!current_token().is_identifier())
                raise_unexpected(env, "parse_def first arg identifier");
            args->push(parse_identifier(env, locals));
            while (current_token().is_comma()) {
                advance();
                if (!current_token().is_identifier())
                    raise_unexpected(env, "parse_def 2nd+ arg identifier");
                args->push(parse_identifier(env, locals));
            };
            if (!current_token().is_rparen())
                raise_unexpected(env, "parse_def rparen");
            advance();
        } else if (current_token().is_identifier()) {
            args->push(parse_identifier(env, locals));
            while (current_token().is_comma()) {
                advance();
                if (!current_token().is_identifier())
                    raise_unexpected(env, "parse_def 2nd+ arg identifier");
                args->push(parse_identifier(env, locals));
            };
        }
        auto body = static_cast<BlockNode *>(parse_body(env, locals));
        return new DefNode { name, args, body };
    };

    Node *parse_body(Env *env, Vector<SymbolValue *> *locals) {
        auto body = new BlockNode {};
        current_token().validate(env);
        skip_newlines();
        while (!current_token().is_eof() && !current_token().is_end_keyword()) {
            auto exp = parse_expression(env, LOWEST, locals);
            body->add_node(exp);
            current_token().validate(env);
            next_expression(env);
        }
        if (!current_token().is_end_keyword())
            raise_unexpected(env, "parse_body end");
        advance();
        return body;
    }

    Node *parse_lit(Env *env, Vector<SymbolValue *> *locals) {
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

    Node *parse_string(Env *env, Vector<SymbolValue *> *locals) {
        auto lit = new StringNode { new StringValue { env, current_token().literal() } };
        advance();
        return lit;
    };

    Node *parse_identifier(Env *env, Vector<SymbolValue *> *locals) {
        bool is_lvar = false;
        auto name_symbol = SymbolValue::intern(env, current_token().literal());
        for (auto local : *locals) {
            if (local == name_symbol) {
                is_lvar = true;
                break;
            }
        }
        auto identifier = new IdentifierNode { current_token(), is_lvar };
        advance();
        return identifier;
    };

    Node *parse_infix_expression(Env *env, Node *left, Vector<SymbolValue *> *locals) {
        auto op = current_token();
        auto precedence = get_precedence();
        advance();
        auto right = parse_expression(env, precedence, locals);
        auto node = new CallNode {
            left,
            op.type_value(env),
        };
        node->add_arg(right);
        return node;
    };

    Node *parse_assignment_expression(Env *env, Node *left, Vector<SymbolValue *> *locals) {
        auto left_identifier = static_cast<IdentifierNode *>(left);
        if (left_identifier->token_type() == Token::Type::Identifier)
            locals->push(SymbolValue::intern(env, left_identifier->name()));
        advance();
        auto right = parse_expression(env, ASSIGNMENT, locals);
        return new AssignmentNode {
            left_identifier,
            right,
        };
    };

    Node *parse_grouped_expression(Env *env, Vector<SymbolValue *> *locals) {
        advance();
        auto exp = parse_expression(env, LOWEST, locals);
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

    using parse_null_fn = Node *(Parser::*)(Env *, Vector<SymbolValue *> *);
    using parse_left_fn = Node *(Parser::*)(Env *, Node *, Vector<SymbolValue *> *);

    parse_null_fn null_denotation(Token::Type type) {
        using Type = Token::Type;
        switch (type) {
        case Type::DefKeyword:
            return &Parser::parse_def;
        case Type::LParen:
            return &Parser::parse_grouped_expression;
        case Type::ClassVariable:
        case Type::Constant:
        case Type::GlobalVariable:
        case Type::Identifier:
        case Type::InstanceVariable:
            return &Parser::parse_identifier;
        case Type::Integer:
        case Type::Float:
            return &Parser::parse_lit;
        case Type::String:
            return &Parser::parse_string;
        default:
            return nullptr;
        }
    };

    parse_left_fn left_denotation(Token::Type type) {
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

    void next_expression(Env *env) {
        auto token = current_token();
        if (!token.is_eol() && !token.is_eof())
            raise_unexpected(env, "end-of-line");
        skip_newlines();
    }

    void skip_newlines() {
        while (current_token().is_eol())
            advance();
    }

    void raise_unexpected(Env *env, const char *expected) {
        auto line = current_token().line() + 1;
        auto type = current_token().type_value(env);
#ifdef NAT_DEBUG_PARSER
        printf("DEBUG: %s expected\n", expected);
#endif
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
