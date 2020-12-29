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
    Value *left;
    switch (m_identifier->type()) {
    case Node::Type::MultipleAssignment: {
        auto list = static_cast<MultipleAssignmentNode *>(m_identifier);
        auto sexp = new SexpValue { env, { SymbolValue::intern(env, "masgn") } };
        auto array = new SexpValue { env, { SymbolValue::intern(env, "array") } };
        for (auto identifier : *(list->nodes())) {
            assert(identifier->type() == Node::Type::Identifier);
            array->push(static_cast<IdentifierNode *>(identifier)->to_sexp(env));
        }
        sexp->push(array);
        auto value = new SexpValue { env, { SymbolValue::intern(env, "to_ary") } };
        value->push(m_value->to_ruby(env));
        sexp->push(value);
        return sexp;
    }
    case Node::Type::Identifier: {
        auto sexp = static_cast<IdentifierNode *>(m_identifier)->to_sexp(env);
        sexp->push(m_value->to_ruby(env));
        return sexp;
    }
    default:
        NAT_UNREACHABLE();
    }
}

Value *Parser::BlockNode::to_ruby(Env *env) {
    auto *array = new SexpValue { env, { SymbolValue::intern(env, "block") } };
    for (auto node : m_nodes) {
        array->push(node->to_ruby(env));
    }
    return array;
}

Value *Parser::BlockPassNode::to_ruby(Env *env) {
    auto *sexp = new SexpValue { env, { SymbolValue::intern(env, "block_pass") } };
    sexp->push(m_node->to_ruby(env));
    return sexp;
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

Value *Parser::IterNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, {
                                         SymbolValue::intern(env, "iter"),
                                         m_call->to_ruby(env),
                                     } };
    if (m_args->is_empty())
        sexp->push(new IntegerValue { env, 0 });
    else
        sexp->push(build_args_sexp(env));
    if (!m_body->is_empty()) {
        if (m_body->has_one_node())
            sexp->push((*m_body->nodes())[0]->to_ruby(env));
        else
            sexp->push(m_body->to_ruby(env));
    }
    return sexp;
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

Value *Parser::MultipleAssignmentNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, { SymbolValue::intern(env, "masgn") } };
    for (auto node : m_nodes) {
        if (node->type() == Node::Type::Identifier) {
            sexp->push(static_cast<IdentifierNode *>(node)->to_symbol(env));
        } else {
            NAT_NOT_YET_IMPLEMENTED(); // maybe not needed?
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
