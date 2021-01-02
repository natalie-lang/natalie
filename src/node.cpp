#include "natalie/parser.hpp"

namespace Natalie {

Value *ArrayNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "array") } };
    for (auto node : m_nodes) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value *AssignmentNode::to_ruby(Env *env) {
    const char *assignment_type;
    Value *left;
    switch (m_identifier->type()) {
    case Node::Type::MultipleAssignment: {
        auto masgn = static_cast<MultipleAssignmentNode *>(m_identifier);
        auto sexp = masgn->to_ruby_with_array(env);
        auto value = new SexpValue { env, this, { SymbolValue::intern(env, "to_ary") } };
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

Value *AttrAssignNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "attrasgn"),
            m_receiver->to_ruby(env),
            SymbolValue::intern(env, m_message),
        }
    };

    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

Value *BlockNode::to_ruby(Env *env) {
    auto *array = new SexpValue { env, this, { SymbolValue::intern(env, "block") } };
    for (auto node : m_nodes) {
        array->push(node->to_ruby(env));
    }
    return array;
}

Value *BlockPassNode::to_ruby(Env *env) {
    auto *sexp = new SexpValue { env, this, { SymbolValue::intern(env, "block_pass") } };
    sexp->push(m_node->to_ruby(env));
    return sexp;
}

Value *BreakNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "break") } };
    if (m_arg)
        sexp->push(m_arg->to_ruby(env));
    return sexp;
}

Value *CallNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "call"),
            m_receiver->to_ruby(env),
            SymbolValue::intern(env, m_message),
        }
    };

    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

Value *CaseNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "case"),
            m_subject->to_ruby(env),
        }
    };
    for (auto when_node : m_when_nodes) {
        sexp->push(when_node->to_ruby(env));
    }
    if (m_else_node) {
        if (m_else_node->has_one_node())
            sexp->push((*m_else_node->nodes())[0]->to_ruby(env));
        else
            sexp->push(m_else_node->to_ruby(env));
    } else {
        sexp->push(env->nil_obj());
    }
    return sexp;
}

Value *CaseWhenNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "when"),
            m_condition->to_ruby(env),
        }
    };
    for (auto node : *m_body->nodes()) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value *ClassNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "class"), SymbolValue::intern(env, m_name->name()), m_superclass->to_ruby(env) } };
    if (!m_body->is_empty()) {
        for (auto node : *(m_body->nodes())) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

Value *Colon2Node::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "colon2"),
            m_left->to_ruby(env),
            SymbolValue::intern(env, m_name),
        }
    };
}

Value *Colon3Node::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "colon3"),
            SymbolValue::intern(env, m_name),
        }
    };
}

Value *ConstantNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "const"),
            SymbolValue::intern(env, m_token.literal()),
        }
    };
}

Value *DefNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "defn"),
            SymbolValue::intern(env, m_name->name()),
            build_args_sexp(env),
        }
    };
    if (m_body->is_empty()) {
        sexp->push(new SexpValue { env, this, { SymbolValue::intern(env, "nil") } });
    } else {
        for (auto node : *(m_body->nodes())) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

SexpValue *DefNode::build_args_sexp(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "args") } };
    for (auto arg : *m_args) {
        switch (arg->type()) {
        case Node::Type::Identifier:
            sexp->push(SymbolValue::intern(env, static_cast<IdentifierNode *>(arg)->name()));
            break;
        case Node::Type::MultipleAssignment:
            sexp->push(arg->to_ruby(env));
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
    return sexp;
}

Value *EvaluateToStringNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "evstr"), m_node->to_ruby(env) } };
}

Value *FalseNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "false") } };
}

Value *HashNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "hash") } };
    for (auto node : m_nodes) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value *IdentifierNode::to_ruby(Env *env) {
    switch (token_type()) {
    case Token::Type::BareName:
        if (m_is_lvar) {
            return new SexpValue { env, this, { SymbolValue::intern(env, "lvar"), SymbolValue::intern(env, name()) } };
        } else {
            return new SexpValue { env, this, { SymbolValue::intern(env, "call"), env->nil_obj(), SymbolValue::intern(env, name()) } };
        }
    case Token::Type::ClassVariable:
        return new SexpValue { env, this, { SymbolValue::intern(env, "cvar"), SymbolValue::intern(env, name()) } };
    case Token::Type::Constant:
        return new SexpValue { env, this, { SymbolValue::intern(env, "const"), SymbolValue::intern(env, name()) } };
    case Token::Type::GlobalVariable:
        return new SexpValue { env, this, { SymbolValue::intern(env, "gvar"), SymbolValue::intern(env, name()) } };
    case Token::Type::InstanceVariable:
        return new SexpValue { env, this, { SymbolValue::intern(env, "ivar"), SymbolValue::intern(env, name()) } };
    default:
        NAT_NOT_YET_IMPLEMENTED();
    }
}

SexpValue *IdentifierNode::to_sexp(Env *env) {
    return new SexpValue {
        env,
        this,
        { assignment_type(env), SymbolValue::intern(env, name()) }
    };
}

Value *IfNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "if"),
            m_condition->to_ruby(env),
            m_true_expr->to_ruby(env),
            m_false_expr->to_ruby(env),
        }
    };
}

Value *IterNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "iter"),
            m_call->to_ruby(env),
        }
    };
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

SexpValue *IterNode::build_args_sexp(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "args") } };
    for (auto arg : *m_args) {
        switch (arg->type()) {
        case Node::Type::Identifier:
            sexp->push(SymbolValue::intern(env, static_cast<IdentifierNode *>(arg)->name()));
            break;
        case Node::Type::MultipleAssignment:
            sexp->push(arg->to_ruby(env));
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
    return sexp;
}

Value *InterpolatedStringNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "dstr") } };
    for (size_t i = 0; i < m_nodes.size(); i++) {
        auto node = m_nodes[i];
        if (i == 0 && node->type() == Node::Type::String)
            sexp->push(static_cast<StringNode *>(node)->value());
        else
            sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value *LiteralNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "lit"), m_value } };
}

Value *LogicalAndNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "and"),
            m_left->to_ruby(env),
            m_right->to_ruby(env),
        }
    };

    return sexp;
}

Value *LogicalOrNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "or"),
            m_left->to_ruby(env),
            m_right->to_ruby(env),
        }
    };

    return sexp;
}

Value *ModuleNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "module"), SymbolValue::intern(env, m_name->name()) } };
    if (!m_body->is_empty()) {
        for (auto node : *(m_body->nodes())) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

Value *MultipleAssignmentNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "masgn") } };
    for (auto node : m_nodes) {
        if (node->type() == Node::Type::Identifier) {
            sexp->push(static_cast<IdentifierNode *>(node)->to_symbol(env));
        } else {
            NAT_NOT_YET_IMPLEMENTED(); // maybe not needed?
        }
    }
    return sexp;
}

ArrayValue *MultipleAssignmentNode::to_ruby_with_array(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "masgn") } };
    auto array = new SexpValue { env, this, { SymbolValue::intern(env, "array") } };
    for (auto identifier : m_nodes) {
        switch (identifier->type()) {
        case Node::Type::Identifier:
            array->push(static_cast<IdentifierNode *>(identifier)->to_sexp(env));
            break;
        case Node::Type::MultipleAssignment:
            array->push(static_cast<MultipleAssignmentNode *>(identifier)->to_ruby_with_array(env));
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
    sexp->push(array);
    return sexp;
}

Value *NextNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "next") } };
    if (m_arg)
        sexp->push(m_arg->to_ruby(env));
    return sexp;
}

Value *NilNode::to_ruby(Env *env) {
    return env->nil_obj();
}

Value *NilSexpNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "nil") } };
}

Value *NotNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "not"), m_expression->to_ruby(env) } };
}

Value *RangeNode::to_ruby(Env *env) {
    if (m_first->type() == Node::Type::Literal && static_cast<LiteralNode *>(m_first)->value_type() == Value::Type::Integer && m_last->type() == Node::Type::Literal && static_cast<LiteralNode *>(m_last)->value_type() == Value::Type::Integer) {
        return new SexpValue {
            env,
            this,
            { SymbolValue::intern(env, "lit"), new RangeValue { env, static_cast<LiteralNode *>(m_first)->value(), static_cast<LiteralNode *>(m_last)->value(), m_exclude_end } }
        };
    }
    return new SexpValue {
        env,
        this,
        { SymbolValue::intern(env, m_exclude_end ? "dot3" : "dot2"), m_first->to_ruby(env), m_last->to_ruby(env) }
    };
}

Value *RegexpNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "lit"), m_value } };
}

Value *ReturnNode::to_ruby(Env *env) {
    if (m_arg) {
        return new SexpValue { env, this, { SymbolValue::intern(env, "return"), m_arg->to_ruby(env) } };
    }
    return new SexpValue { env, this, { SymbolValue::intern(env, "return") } };
}

Value *SplatNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "splat"), m_node->to_ruby(env) } };
}

Value *StabbyProcNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "lambda") } };
}

Value *StringNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "str"), m_value } };
}

Value *SymbolNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "lit"), m_value } };
}

Value *TrueNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "true") } };
}

Value *YieldNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "yield") } };
    if (m_args.is_empty())
        return sexp;
    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

}
