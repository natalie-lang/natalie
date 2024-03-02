#include "natalie.hpp"

namespace Natalie {

void GlobalVariableInfo::set_object(Env *env, Object *object) {
    if (m_write_hook) {
        m_object = m_write_hook(env, object, *this);
    } else {
        m_object = object;
    }
}

Value GlobalVariableInfo::object(Env *env) {
    if (m_read_hook)
        return m_read_hook(env, *this);
    return m_object;
}

void GlobalVariableInfo::visit_children(Visitor &visitor) {
    visitor.visit(m_object);
}

}
