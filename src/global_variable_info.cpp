#include "natalie.hpp"

namespace Natalie {

void GlobalVariableInfo::set_object(Env *env, Value value) {
    if (m_write_hook) {
        m_value = m_write_hook(env, value, *this);
    } else {
        m_value = value;
    }
}

Optional<Value> GlobalVariableInfo::object(Env *env) {
    if (m_read_hook)
        return m_read_hook(env, *this);
    return m_value;
}

const String &GlobalVariableInfo::name() const {
    return m_name->string();
}

void GlobalVariableInfo::visit_children(Visitor &visitor) const {
    visitor.visit(m_name);
    if (m_value)
        visitor.visit(m_value.value());
}

}
