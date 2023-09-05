#include "natalie.hpp"

namespace Natalie {

void MethodInfo::visit_children(Cell::Visitor &visitor) {
    visitor.visit(m_method);
}

}
