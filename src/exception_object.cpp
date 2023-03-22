#include "natalie.hpp"

namespace Natalie {

Value ExceptionObject::initialize(Env *env, Value message) {
    if (message != nullptr)
        set_message(message);
    return this;
}

// static exception
Value ExceptionObject::exception(Env *env, Value message, ClassObject *klass) {
    auto exc = new ExceptionObject { klass };
    return exc->initialize(env, message);
}

Value ExceptionObject::exception(Env *env, Value val) {
    if (!val) return this;
    if (val == this) return this;
    auto exc = clone(env)->as_exception();
    exc->set_message(val);
    return exc;
}

Value ExceptionObject::inspect(Env *env) {
    auto klassname = m_klass->inspect_str();
    auto msgstr = this->send(env, "to_s"_s);
    assert(msgstr);
    msgstr->assert_type(env, Object::Type::String, "String");
    if (msgstr->as_string()->is_empty())
        return new StringObject { klassname };
    if (msgstr->as_string()->include(env, new StringObject { "\n" }))
        return StringObject::format("#<{}: {}>", klassname, klassname);
    return StringObject::format("#<{}: {}>", klassname, msgstr);
}

StringObject *ExceptionObject::to_s(Env *env) {
    if (m_message == nullptr || m_message->is_nil()) {
        return new StringObject { m_klass->inspect_str() };
    } else if (m_message->is_string()) {
        return m_message->as_string();
    }
    auto msgstr = m_message->send(env, "to_s"_s);
    msgstr->assert_type(env, Object::Type::String, "String");
    return msgstr->as_string();
}

Value ExceptionObject::message(Env *env) {
    return this->send(env, "to_s"_s);
}

Value ExceptionObject::backtrace(Env *env) {
    if (m_backtrace)
        return m_backtrace->to_ruby_array();

    return NilObject::the();
}

Value ExceptionObject::match_rescue_array(Env *env, Value ary) {
    // NOTE: Even though LocalJumpError is a StandardError, we only
    // want it to match if it was explicitly given in the array,
    // since it's special and controls how `break` works.
    bool match_local_jump_error = false;
    for (auto klass : *ary->as_array()) {
        if (klass->is_class()) {
            auto name = klass->as_class()->name();
            if (name && name.value() == "LocalJumpError")
                match_local_jump_error = true;
        }
    }
    if (!match_local_jump_error && m_local_jump_error_type != LocalJumpErrorType::None)
        return FalseObject::the();

    for (auto klass : *ary->as_array()) {
        if (klass->send(env, "==="_s, { this })->is_truthy())
            return TrueObject::the();
    }
    return FalseObject::the();
}

bool ExceptionObject::is_local_jump_error_with_break_point(nat_int_t match_break_point) const {
    if (m_local_jump_error_type == LocalJumpErrorType::None) return false;
    if (m_break_point == 0) return false;
    return m_break_point == match_break_point;
}

void ExceptionObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_message);
    visitor.visit(m_backtrace);
}

}
