#include "natalie_parser/node.hpp"

namespace NatalieParser {

AliasNode::~AliasNode() {
    delete m_new_name;
    delete m_existing_name;
}

BeginNode::~BeginNode() {
    delete m_body;
    delete m_else_body;
    delete m_ensure_body;
    for (auto node : m_rescue_nodes)
        delete node;
}

BeginRescueNode::~BeginRescueNode() {
    delete m_name;
    delete m_body;
    for (auto node : m_exceptions)
        delete node;
}

ClassNode::~ClassNode() {
    delete m_name;
    delete m_superclass;
    delete m_body;
}

MatchNode::~MatchNode() {
    delete m_regexp;
    delete m_arg;
}

Node *BeginRescueNode::name_to_node() {
    assert(m_name);
    return new AssignmentNode {
        token(),
        m_name,
        new IdentifierNode {
            Token { Token::Type::GlobalVariable, "$!", file(), line(), column() },
            false },
    };
}

void MultipleAssignmentNode::add_locals(TM::Hashmap<const char *> &locals) {
    for (auto node : m_nodes) {
        switch (node->type()) {
        case Node::Type::Identifier: {
            auto identifier = static_cast<IdentifierNode *>(node);
            identifier->add_to_locals(locals);
            break;
        }
        case Node::Type::Call:
        case Node::Type::Colon2:
        case Node::Type::Colon3:
            break;
        case Node::Type::Splat: {
            auto splat = static_cast<SplatNode *>(node);
            if (splat->node() && splat->node()->type() == Node::Type::Identifier) {
                auto identifier = static_cast<IdentifierNode *>(splat->node());
                identifier->add_to_locals(locals);
            }
            break;
        }
        case Node::Type::MultipleAssignment:
            static_cast<MultipleAssignmentNode *>(node)->add_locals(locals);
            break;
        default:
            TM_UNREACHABLE();
        }
    }
}

}
