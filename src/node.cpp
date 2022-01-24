#include "natalie/node.hpp"

namespace Natalie {

void AliasNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_new_name);
    visitor.visit(m_existing_name);
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

void ClassNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_name);
    visitor.visit(m_superclass);
    visitor.visit(m_body);
}

void DefNode::visit_children(Visitor &visitor) {
    NodeWithArgs::visit_children(visitor);
    visitor.visit(m_self_node);
    visitor.visit(m_body);
}

void MatchNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_regexp);
    visitor.visit(m_arg);
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

}
