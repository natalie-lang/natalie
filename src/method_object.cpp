#include "natalie.hpp"

namespace Natalie {

ProcObject *MethodObject::to_proc(Env *env) {
    auto closure = Env::create();
    closure->set_lexical_scope(m_method->lexical_scope());
    if (m_method->get_file()) {
        closure->set_file(m_method->get_file().value().c_str());
        closure->set_line(m_method->get_line().value_or(0));
    }
    closure->var_set("method", 0, true, this);
    auto fn = [](Env *env, Value self, Args &&args, Block *block) -> Value {
        auto method = env->outer()->var_get("method", 0).as_method();
        return method->call(env, std::move(args), block);
    };
    auto block = Block::create(*closure, m_object, m_method->lexical_scope(), fn, m_method->arity(), false, Block::BlockType::MethodProc, m_method->parameters());
    return ProcObject::create(block);
}

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

    return ArrayObject::create({ StringObject::create(method->get_file().value()), Value::integer(static_cast<nat_int_t>(method->get_line().value())) });
}

Value MethodObject::unbind(Env *env) {
    if (m_object->singleton_class()) {
        return UnboundMethodObject::create(m_object->singleton_class(), m_method);
    } else {
        return UnboundMethodObject::create(m_object.klass(), m_method);
    }
}

}
