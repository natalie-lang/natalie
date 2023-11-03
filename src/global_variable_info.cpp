#include "natalie.hpp"

namespace Natalie {

void GlobalVariableInfo::visit_children(Visitor &visitor) {
    visitor.visit(m_object);
}

}
