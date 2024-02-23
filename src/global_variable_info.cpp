#include "natalie.hpp"

namespace Natalie {

Value GlobalVariableInfo::object(Env *env) {
    if (m_read_hook)
        return m_read_hook(env, *this);
    return m_object;
}

void GlobalVariableInfo::visit_children(Visitor &visitor) {
    visitor.visit(m_object);
}

}
