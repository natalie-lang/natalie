#include "natalie/node.hpp"

namespace Natalie {

Value AliasNode::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            "alias"_s,
            m_new_name->to_ruby(env),
            m_existing_name->to_ruby(env),
        }
    };
}

void AliasNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_new_name);
    visitor.visit(m_existing_name);
}

Value ArgNode::to_ruby(Env *env) {
    if (m_value) {
        return new SexpObject {
            env,
            this,
            {
                "lasgn"_s,
                SymbolObject::intern(*m_name),
                m_value->to_ruby(env),
            }
        };
    } else {
        String name;
        if (m_name)
            name = String(*m_name);
        else
            name = String();
        if (m_splat) {
            name.prepend_char('*');
        } else if (m_kwsplat) {
            name.prepend_char('*');
            name.prepend_char('*');
        } else if (m_block_arg) {
            name.prepend_char('&');
        }
        return SymbolObject::intern(name.c_str());
    }
}

Value ArrayNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "array"_s } };
    for (auto node : m_nodes) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value AssignmentNode::to_ruby(Env *env) {
    const char *assignment_type;
    Value left;
    switch (m_identifier->type()) {
    case Node::Type::MultipleAssignment: {
        auto masgn = static_cast<MultipleAssignmentNode *>(m_identifier);
        auto sexp = masgn->to_ruby_with_array(env);
        auto value = new SexpObject { env, this, { "to_ary"_s } };
        value->push(m_value->to_ruby(env));
        sexp->push(value);
        return sexp;
    }
    case Node::Type::Identifier: {
        auto sexp = static_cast<IdentifierNode *>(m_identifier)->to_assignment_sexp(env);
        sexp->push(m_value->to_ruby(env));
        return sexp;
    }
    default:
        NAT_UNREACHABLE();
    }
}

Value AttrAssignNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "attrasgn"_s,
            m_receiver->to_ruby(env),
            SymbolObject::intern(*m_message),
        }
    };

    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

Value BeginNode::to_ruby(Env *env) {
    assert(m_body);
    auto *sexp = new SexpObject { env, this, { "rescue"_s } };
    if (!m_body->is_empty())
        sexp->push(m_body->without_unnecessary_nesting()->to_ruby(env));
    for (auto rescue_node : m_rescue_nodes) {
        sexp->push(rescue_node->to_ruby(env));
    }
    if (m_else_body)
        sexp->push(m_else_body->without_unnecessary_nesting()->to_ruby(env));
    if (m_ensure_body) {
        if (m_rescue_nodes.is_empty())
            (*sexp)[0] = "ensure"_s;
        else
            sexp = new SexpObject { env, this, { "ensure"_s, sexp } };
        sexp->push(m_ensure_body->without_unnecessary_nesting()->to_ruby(env));
    }
    return sexp;
}

void BeginNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_body);
    visitor.visit(m_else_body);
    visitor.visit(m_ensure_body);
    for (auto node : m_rescue_nodes) {
        visitor.visit(node);
    }
}

Node *BeginRescueNode::name_to_node() {
    assert(m_name);
    return new AssignmentNode {
        token(),
        m_name,
        new IdentifierNode {
            new Token { Token::Type::GlobalVariable, "$!", file(), line(), column() },
            false },
    };
}

void BeginRescueNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_name);
    visitor.visit(m_body);
    for (auto node : m_exceptions) {
        visitor.visit(node);
    }
}

Value BeginRescueNode::to_ruby(Env *env) {
    auto array = new ArrayNode { token() };
    for (auto exception_node : m_exceptions) {
        array->add_node(exception_node);
    }
    if (m_name)
        array->add_node(name_to_node());
    auto *rescue_node = new SexpObject {
        env,
        this,
        {
            "resbody"_s,
            array->to_ruby(env),
        }
    };
    for (auto node : m_body->nodes()) {
        rescue_node->push(node->to_ruby(env));
    }
    return rescue_node;
}

Value BlockNode::to_ruby(Env *env) {
    return to_ruby_with_name(env, "block");
}

Value BlockNode::to_ruby_with_name(Env *env, const char *name) {
    auto *array = new SexpObject { env, this, { SymbolObject::intern(name) } };
    for (auto node : m_nodes) {
        array->push(node->to_ruby(env));
    }
    return array;
}

Value BlockPassNode::to_ruby(Env *env) {
    auto *sexp = new SexpObject { env, this, { "block_pass"_s } };
    sexp->push(m_node->to_ruby(env));
    return sexp;
}

Value BreakNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "break"_s } };
    if (m_arg)
        sexp->push(m_arg->to_ruby(env));
    return sexp;
}

Value CallNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "call"_s,
            m_receiver->to_ruby(env),
            SymbolObject::intern(*m_message),
        }
    };

    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

Value CaseNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "case"_s,
            m_subject->to_ruby(env),
        }
    };
    for (auto when_node : m_when_nodes) {
        sexp->push(when_node->to_ruby(env));
    }
    if (m_else_node) {
        sexp->push(m_else_node->without_unnecessary_nesting()->to_ruby(env));
    } else {
        sexp->push(NilObject::the());
    }
    return sexp;
}

Value CaseWhenNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "when"_s,
            m_condition->to_ruby(env),
        }
    };
    for (auto node : m_body->nodes()) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value ClassNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "class"_s, SymbolObject::intern(*m_name->name()), m_superclass->to_ruby(env) } };
    if (!m_body->is_empty()) {
        for (auto node : m_body->nodes()) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

void ClassNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_name);
    visitor.visit(m_superclass);
    visitor.visit(m_body);
}

Value Colon2Node::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            "colon2"_s,
            m_left->to_ruby(env),
            SymbolObject::intern(*m_name),
        }
    };
}

Value Colon3Node::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            "colon3"_s,
            SymbolObject::intern(*m_name),
        }
    };
}

Value ConstantNode::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            "const"_s,
            SymbolObject::intern(*m_token->literal_string()),
        }
    };
}

Value DefinedNode::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            "defined"_s,
            m_arg->to_ruby(env),
        }
    };
}

Value DefNode::to_ruby(Env *env) {
    SexpObject *sexp;
    if (m_self_node) {
        sexp = new SexpObject {
            env,
            this,
            {
                "defs"_s,
                m_self_node->to_ruby(env),
                SymbolObject::intern(*m_name),
                build_args_sexp(env),
            }
        };
    } else {
        sexp = new SexpObject {
            env,
            this,
            {
                "defn"_s,
                SymbolObject::intern(*m_name),
                build_args_sexp(env),
            }
        };
    }
    if (m_body->is_empty()) {
        sexp->push(new SexpObject { env, this, { "nil"_s } });
    } else {
        for (auto node : m_body->nodes()) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

SexpObject *DefNode::build_args_sexp(Env *env) {
    auto sexp = new SexpObject { env, this, { "args"_s } };
    for (auto arg : m_args) {
        switch (arg->type()) {
        case Node::Type::Arg:
        case Node::Type::KeywordArg:
        case Node::Type::MultipleAssignment:
            sexp->push(arg->to_ruby(env));
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
    return sexp;
}

void DefNode::visit_children(Visitor &visitor) {
    NodeWithArgs::visit_children(visitor);
    visitor.visit(m_self_node);
    visitor.visit(m_body);
}

Value EvaluateToStringNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "evstr"_s, m_node->to_ruby(env) } };
}

Value FalseNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "false"_s } };
}

Value HashNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "hash"_s } };
    for (auto node : m_nodes) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value IdentifierNode::to_ruby(Env *env) {
    switch (token_type()) {
    case Token::Type::BareName:
        if (m_is_lvar) {
            return new SexpObject { env, this, { "lvar"_s, SymbolObject::intern(*name()) } };
        } else {
            return new SexpObject { env, this, { "call"_s, NilObject::the(), SymbolObject::intern(*name()) } };
        }
    case Token::Type::ClassVariable:
        return new SexpObject { env, this, { "cvar"_s, SymbolObject::intern(*name()) } };
    case Token::Type::Constant:
        return new SexpObject { env, this, { "const"_s, SymbolObject::intern(*name()) } };
    case Token::Type::GlobalVariable: {
        auto ref = nth_ref();
        if (ref > 0)
            return new SexpObject { env, this, { "nth_ref"_s, Value::integer(ref) } };
        else
            return new SexpObject { env, this, { "gvar"_s, SymbolObject::intern(*name()) } };
    }
    case Token::Type::InstanceVariable:
        return new SexpObject { env, this, { "ivar"_s, SymbolObject::intern(*name()) } };
    default:
        NAT_NOT_YET_IMPLEMENTED();
    }
}

SexpObject *IdentifierNode::to_assignment_sexp(Env *env) {
    return new SexpObject {
        env,
        this,
        { assignment_type(env), SymbolObject::intern(*name()) }
    };
}

Value IfNode::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            "if"_s,
            m_condition->to_ruby(env),
            m_true_expr->to_ruby(env),
            m_false_expr->to_ruby(env),
        }
    };
}

Value IterNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "iter"_s,
            m_call->to_ruby(env),
        }
    };
    if (m_args.is_empty())
        sexp->push(Value::integer(0));
    else
        sexp->push(build_args_sexp(env));
    if (!m_body->is_empty()) {
        if (m_body->has_one_node())
            sexp->push(m_body->nodes()[0]->to_ruby(env));
        else
            sexp->push(m_body->to_ruby(env));
    }
    return sexp;
}

SexpObject *IterNode::build_args_sexp(Env *env) {
    auto sexp = new SexpObject { env, this, { "args"_s } };
    for (auto arg : m_args) {
        switch (arg->type()) {
        case Node::Type::Arg:
        case Node::Type::KeywordArg:
        case Node::Type::MultipleAssignment:
            sexp->push(arg->to_ruby(env));
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
    return sexp;
}

Value InterpolatedRegexpNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "dregx"_s } };
    for (size_t i = 0; i < m_nodes.size(); i++) {
        auto node = m_nodes[i];
        if (i == 0 && node->type() == Node::Type::String)
            sexp->push(static_cast<StringNode *>(node)->to_string_value());
        else
            sexp->push(node->to_ruby(env));
    }
    if (m_options != 0)
        sexp->push(Value::integer(m_options));
    return sexp;
}

Value InterpolatedShellNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "dxstr"_s } };
    for (size_t i = 0; i < m_nodes.size(); i++) {
        auto node = m_nodes[i];
        if (i == 0 && node->type() == Node::Type::String)
            sexp->push(static_cast<StringNode *>(node)->to_string_value());
        else
            sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value InterpolatedStringNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "dstr"_s } };
    for (size_t i = 0; i < m_nodes.size(); i++) {
        auto node = m_nodes[i];
        if (i == 0 && node->type() == Node::Type::String)
            sexp->push(static_cast<StringNode *>(node)->to_string_value());
        else
            sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value KeywordArgNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "kwarg"_s,
            SymbolObject::intern(*m_name),
        }
    };
    if (m_value)
        sexp->push(m_value->to_ruby(env));
    return sexp;
}

Value KeywordSplatNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "hash"_s } };
    auto kwsplat_sexp = new SexpObject { env, this, { "kwsplat"_s } };
    if (m_node)
        kwsplat_sexp->push(m_node->to_ruby(env));
    sexp->push(kwsplat_sexp);
    return sexp;
}

Value IntegerNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "lit"_s, Value::integer(m_number) } };
}

Value FloatNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "lit"_s, new FloatObject { m_number } } };
}

Value LogicalAndNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "and"_s,
            m_left->to_ruby(env),
            m_right->to_ruby(env),
        }
    };

    return sexp;
}

Value LogicalOrNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "or"_s,
            m_left->to_ruby(env),
            m_right->to_ruby(env),
        }
    };

    return sexp;
}

Value MatchNode::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            m_regexp_on_left ? "match2"_s : "match3"_s,
            m_regexp->to_ruby(env),
            m_arg->to_ruby(env),
        }
    };
}

void MatchNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_regexp);
    visitor.visit(m_arg);
}

Value ModuleNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "module"_s, SymbolObject::intern(*m_name->name()) } };
    if (!m_body->is_empty()) {
        for (auto node : m_body->nodes()) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

Value MultipleAssignmentNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "masgn"_s } };
    for (auto node : m_nodes) {
        switch (node->type()) {
        case Node::Type::Arg:
        case Node::Type::MultipleAssignment:
            sexp->push(node->to_ruby(env));
            break;
        default:
            NAT_NOT_YET_IMPLEMENTED(); // maybe not needed?
        }
    }
    return sexp;
}

void MultipleAssignmentNode::add_locals(TM::Hashmap<const char *> &locals) {
    for (auto node : m_nodes) {
        switch (node->type()) {
        case Node::Type::Identifier: {
            auto identifier = static_cast<IdentifierNode *>(node);
            identifier->add_to_locals(locals);
            break;
        }
        case Node::Type::Splat:
            break;
        case Node::Type::SplatAssignment: {
            auto splat = static_cast<SplatAssignmentNode *>(node);
            if (splat->node())
                splat->node()->add_to_locals(locals);
            break;
        }
        case Node::Type::MultipleAssignment:
            static_cast<MultipleAssignmentNode *>(node)->add_locals(locals);
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
}

ArrayObject *MultipleAssignmentNode::to_ruby_with_array(Env *env) {
    auto sexp = new SexpObject { env, this, { "masgn"_s } };
    auto array = new SexpObject { env, this, { "array"_s } };
    for (auto identifier : m_nodes) {
        switch (identifier->type()) {
        case Node::Type::Identifier:
            array->push(static_cast<IdentifierNode *>(identifier)->to_assignment_sexp(env));
            break;
        case Node::Type::Splat:
        case Node::Type::SplatAssignment:
            array->push(identifier->to_ruby(env));
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

Value NextNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "next"_s } };
    if (m_arg)
        sexp->push(m_arg->to_ruby(env));
    return sexp;
}

Value NilNode::to_ruby(Env *env) {
    return NilObject::the();
}

Value NilSexpNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "nil"_s } };
}

Value NotNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "not"_s, m_expression->to_ruby(env) } };
}

Value OpAssignNode::to_ruby(Env *env) {
    auto sexp = m_name->to_assignment_sexp(env);
    assert(m_op);
    auto call = new CallNode { token(), m_name, m_op };
    call->add_arg(m_value);
    sexp->push(call->to_ruby(env));
    return sexp;
}

Value OpAssignAccessorNode::to_ruby(Env *env) {
    if (*m_message == "[]=") {
        auto arg_list = new SexpObject {
            env,
            this,
            {
                "arglist"_s,
            }
        };
        for (auto arg : m_args) {
            arg_list->push(arg->to_ruby(env));
        }
        return new SexpObject {
            env,
            this,
            {
                "op_asgn1"_s,
                m_receiver->to_ruby(env),
                arg_list,
                SymbolObject::intern(*m_op),
                m_value->to_ruby(env),
            }
        };
    } else {
        assert(m_args.is_empty());
        return new SexpObject {
            env,
            this,
            {
                "op_asgn2"_s,
                m_receiver->to_ruby(env),
                SymbolObject::intern(*m_message),
                SymbolObject::intern(*m_op),
                m_value->to_ruby(env),
            }
        };
    }
}

Value OpAssignAndNode::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            "op_asgn_and"_s,
            m_name->to_ruby(env),
            (new AssignmentNode { token(), m_name, m_value })->to_ruby(env),
        },
    };
}

Value OpAssignOrNode::to_ruby(Env *env) {
    return new SexpObject {
        env,
        this,
        {
            "op_asgn_or"_s,
            m_name->to_ruby(env),
            (new AssignmentNode { token(), m_name, m_value })->to_ruby(env),
        },
    };
}

Value RangeNode::to_ruby(Env *env) {
    if (m_first->type() == Node::Type::Integer && m_last->type() == Node::Type::Integer) {
        auto first = static_cast<IntegerNode *>(m_first)->number();
        auto last = static_cast<IntegerNode *>(m_last)->number();
        return new SexpObject {
            env,
            this,
            { "lit"_s,
                new RangeObject {
                    Value::integer(first),
                    Value::integer(last),
                    m_exclude_end } }
        };
    }
    return new SexpObject {
        env,
        this,
        { m_exclude_end ? "dot3"_s : "dot2"_s, m_first->to_ruby(env), m_last->to_ruby(env) }
    };
}

Value RegexpNode::to_ruby(Env *env) {
    auto regexp = new RegexpObject { env, m_pattern->c_str() };
    if (m_options)
        regexp->set_options(*m_options);
    return new SexpObject { env, this, { "lit"_s, regexp } };
}

Value ReturnNode::to_ruby(Env *env) {
    if (m_arg) {
        return new SexpObject { env, this, { "return"_s, m_arg->to_ruby(env) } };
    }
    return new SexpObject { env, this, { "return"_s } };
}

Value SafeCallNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "safe_call"_s,
            m_receiver->to_ruby(env),
            SymbolObject::intern(*m_message),
        }
    };

    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

Value SelfNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "self"_s } };
}

Value SclassNode::to_ruby(Env *env) {
    auto sexp = new SexpObject {
        env,
        this,
        {
            "sclass"_s,
            m_klass->to_ruby(env),
        }
    };
    for (auto node : m_body->nodes()) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

Value ShellNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "xstr"_s, new StringObject(*m_string) } };
}

Value SplatAssignmentNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "splat"_s } };
    if (m_node)
        sexp->push(m_node->to_assignment_sexp(env));
    return sexp;
}

Value SplatNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "splat"_s } };
    if (m_node)
        sexp->push(m_node->to_ruby(env));
    return sexp;
}

Value StabbyProcNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "lambda"_s } };
}

Value StringNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "str"_s, new StringObject(*m_string) } };
}

Value SymbolNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "lit"_s, SymbolObject::intern(*m_name) } };
}

Value TrueNode::to_ruby(Env *env) {
    return new SexpObject { env, this, { "true"_s } };
}

Value SuperNode::to_ruby(Env *env) {
    if (empty_parens()) {
        return new SexpObject { env, this, { "super"_s } };
    } else if (m_args.is_empty()) {
        return new SexpObject { env, this, { "zsuper"_s } };
    }
    auto sexp = new SexpObject { env, this, { "super"_s } };
    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

Value UntilNode::to_ruby(Env *env) {
    auto sexp = WhileNode::to_ruby(env);
    (*sexp->as_array())[0] = "until"_s;
    return sexp;
}

Value WhileNode::to_ruby(Env *env) {
    Value is_pre, body;
    if (m_pre)
        is_pre = TrueObject::the();
    else
        is_pre = FalseObject::the();
    if (m_body->is_empty())
        body = NilObject::the();
    else
        body = m_body->without_unnecessary_nesting()->to_ruby(env);
    return new SexpObject {
        env,
        this,
        {
            "while"_s,
            m_condition->to_ruby(env),
            body,
            is_pre,
        }
    };
}

Value YieldNode::to_ruby(Env *env) {
    auto sexp = new SexpObject { env, this, { "yield"_s } };
    if (m_args.is_empty())
        return sexp;
    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

}
