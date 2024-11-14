#include "natalie.hpp"

namespace Natalie {

void MethodInfo::visit_children(Cell::Visitor &visitor) const {
    visitor.visit(m_method);
}

}
