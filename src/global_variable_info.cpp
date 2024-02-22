#include "natalie.hpp"

namespace Natalie {

void GlobalVariableInfo::set_object(Env *env, Object *object) {
    if (m_assignment_hook) {
        m_assignment_hook(env, &m_object, object);
    } else {
        m_object = object;
    }
}

void GlobalVariableInfo::visit_children(Visitor &visitor) {
    visitor.visit(m_object);
}

}
