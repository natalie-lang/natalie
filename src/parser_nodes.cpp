#include "natalie/parser.hpp"

namespace Natalie {

Value *Parser::ArrayNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, { SymbolValue::intern(env, "array") } };
    for (auto node : m_nodes) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value *Parser::AssignmentNode::to_ruby(Env *env) {
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

Value *Parser::BlockNode::to_ruby(Env *env) {
    auto *array = new SexpValue { env, { SymbolValue::intern(env, "block") } };
    for (auto node : m_nodes) {
        array->push(node->to_ruby(env));
    }
    return array;
}

Value *Parser::CallNode::to_ruby(Env *env) {
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

Value *Parser::ClassNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, { SymbolValue::intern(env, "class"), SymbolValue::intern(env, m_name->name()), m_superclass->to_ruby(env) } };
    if (!m_body->is_empty()) {
        for (auto node : *(m_body->nodes())) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

Value *Parser::ConstantNode::to_ruby(Env *env) {
    return new SexpValue { env, {
                                    SymbolValue::intern(env, "const"),
                                    SymbolValue::intern(env, m_token.literal()),
                                } };
}

Value *Parser::DefNode::to_ruby(Env *env) {
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

Value *Parser::FalseNode::to_ruby(Env *env) {
    return new SexpValue { env, { SymbolValue::intern(env, "false") } };
}

Value *Parser::HashNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, { SymbolValue::intern(env, "hash") } };
    for (auto node : m_nodes) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value *Parser::IdentifierNode::to_ruby(Env *env) {
    if (token_type() == Token::Type::Constant) {
        return new SexpValue { env, { SymbolValue::intern(env, "const"), SymbolValue::intern(env, name()) } };
    } else if (m_is_lvar) {
        return new SexpValue { env, { SymbolValue::intern(env, "lvar"), SymbolValue::intern(env, name()) } };
    } else {
        return new SexpValue { env, { SymbolValue::intern(env, "call"), env->nil_obj(), SymbolValue::intern(env, name()) } };
    }
}

Value *Parser::IfNode::to_ruby(Env *env) {
    return new SexpValue { env, {
                                    SymbolValue::intern(env, "if"),
                                    m_condition->to_ruby(env),
                                    m_true_expr->to_ruby(env),
                                    m_false_expr->to_ruby(env),
                                } };
}

Value *Parser::LiteralNode::to_ruby(Env *env) {
    return new SexpValue { env, { SymbolValue::intern(env, "lit"), m_value } };
}

Value *Parser::ModuleNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, { SymbolValue::intern(env, "module"), SymbolValue::intern(env, m_name->name()) } };
    if (!m_body->is_empty()) {
        for (auto node : *(m_body->nodes())) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

Value *Parser::NilNode::to_ruby(Env *env) {
    return env->nil_obj();
}

Value *Parser::RangeNode::to_ruby(Env *env) {
    if (m_first->type() == Node::Type::Literal && static_cast<LiteralNode *>(m_first)->value_type() == Value::Type::Integer && m_last->type() == Node::Type::Literal && static_cast<LiteralNode *>(m_last)->value_type() == Value::Type::Integer) {
        return new SexpValue {
            env, { SymbolValue::intern(env, "lit"), new RangeValue { env, static_cast<LiteralNode *>(m_first)->value(), static_cast<LiteralNode *>(m_last)->value(), m_exclude_end } }
        };
    }
    return new SexpValue {
        env, { SymbolValue::intern(env, m_exclude_end ? "dot3" : "dot2"), m_first->to_ruby(env), m_last->to_ruby(env) }
    };
}

Value *Parser::ReturnNode::to_ruby(Env *env) {
    if (m_node) {
        return new SexpValue { env, { SymbolValue::intern(env, "return"), m_node->to_ruby(env) } };
    }
    return new SexpValue { env, { SymbolValue::intern(env, "return") } };
}

Value *Parser::SymbolNode::to_ruby(Env *env) {
    return new SexpValue { env, { SymbolValue::intern(env, "lit"), m_value } };
}

Value *Parser::StringNode::to_ruby(Env *env) {
    return new SexpValue { env, { SymbolValue::intern(env, "str"), m_value } };
}

Value *Parser::TrueNode::to_ruby(Env *env) {
    return new SexpValue { env, { SymbolValue::intern(env, "true") } };
}
}
