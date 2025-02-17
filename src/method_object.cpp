#include "natalie.hpp"

namespace Natalie {

bool MethodObject::eq(Env *env, Value other_value) {
    if (other_value.is_method()) {
        auto other = other_value.as_method();
        return m_object == other->m_object && (m_method == other->m_method || (m_method->original_method() && m_method->original_method() == other->m_method));
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

Value MethodObject::hash() const {
    return Value::integer(HashKeyHandler<String>::hash(m_method->original_name()));
}

Value MethodObject::source_location() {
    auto method = m_method;

    if (m_method->original_method())
        method = m_method->original_method();

    if (!method->get_file())
        return Value::nil();

    return new ArrayObject { new StringObject { method->get_file().value() }, Value::integer(static_cast<nat_int_t>(method->get_line().value())) };
}

Value MethodObject::unbind(Env *env) {
    if (m_object->singleton_class()) {
        return new UnboundMethodObject { m_object->singleton_class(), m_method };
    } else {
        return new UnboundMethodObject { m_object.klass(), m_method };
    }
}

}
