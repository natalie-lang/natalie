#include "natalie.hpp"

namespace Natalie {
void LexicalScope::visit_children(Visitor &visitor) const {
    visitor.visit(m_parent);
    visitor.visit(m_module);
}
}
