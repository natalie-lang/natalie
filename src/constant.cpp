#include "natalie.hpp"

namespace Natalie {

void Constant::visit_children(Visitor &visitor) {
    visitor.visit(m_name);
    visitor.visit(m_value);
}

}
