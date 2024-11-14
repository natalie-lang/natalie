#include "natalie/throw_catch_exception.hpp"
#include "natalie.hpp"

namespace Natalie {

void ThrowCatchException::visit_children(Visitor &visitor) const {
    visitor.visit(m_name);
    visitor.visit(m_value);
}

}
