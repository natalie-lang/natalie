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

Value MethodObject::ltlt(Env *env, Value other) {
    return to_proc(env)->ltlt(env, other);
}

Value MethodObject::gtgt(Env *env, Value other) {
    return to_proc(env)->gtgt(env, other);
}

Value MethodObject::source_location() {
    if (!m_method->get_file())
        return NilObject::the();
    return new ArrayObject { new StringObject { m_method->get_file().value() }, Value::integer(static_cast<nat_int_t>(m_method->get_line().value())) };
}

Value MethodObject::unbind(Env *env) {
    if (m_object->singleton_class()) {
        return new UnboundMethodObject { m_object->singleton_class(), m_method };
    } else {
        return new UnboundMethodObject { m_object->klass(), m_method };
    }
}

}
