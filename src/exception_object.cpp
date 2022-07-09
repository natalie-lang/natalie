#include "natalie.hpp"

namespace Natalie {

Value ExceptionObject::initialize(Env *env, Value message) {
    if (!message) {
        auto name = m_klass->inspect_str();
        set_message(new StringObject { *name });
    } else {
        if (!message->is_string()) {
            message = message.send(env, "inspect"_s);
        }
        set_message(message->as_string());
    }
    return this;
}

Value ExceptionObject::inspect(Env *env) {
    return StringObject::format("#<{}: {}>", m_klass->inspect_str(), m_message);
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

Value ExceptionObject::is_local_jump_error_with_break_point(Env *env, nat_int_t match_break_point) {
    if (m_local_jump_error_type != LocalJumpErrorType::Break) return FalseObject::the();
    if (m_break_point == 0) return FalseObject::the();
    return bool_object(m_break_point == match_break_point);
}

void ExceptionObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_message);
    visitor.visit(m_backtrace);
}

}
