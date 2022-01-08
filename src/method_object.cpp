#include "natalie.hpp"

namespace Natalie {

bool MethodObject::eq(Env *env, Value other_value) {
    if (other_value->is_method()) {
        auto other = other_value->as_method();
        return m_object == other->m_object && m_method == other->m_method;
    } else {
        return false;
    }
}

Value MethodObject::unbind(Env *env) {
    if (m_object->singleton_class()) {
        return new UnboundMethodObject { m_object->singleton_class(), m_method };
    } else {
        return new UnboundMethodObject { m_object->klass(), m_method };
    }
}

}
