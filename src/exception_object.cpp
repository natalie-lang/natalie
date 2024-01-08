#include "natalie.hpp"

namespace Natalie {

ExceptionObject *ExceptionObject::create_for_raise(Env *env, Args args, ExceptionObject *current_exception, bool accept_cause) {
    auto kwargs = args.pop_keyword_hash();
    auto cause = accept_cause && kwargs ? kwargs->remove(env, "cause"_s) : nullptr;
    args.ensure_argc_between(env, 0, 3);
    auto klass = args.at(0, nullptr);
    auto message = args.at(1, nullptr);
    auto backtrace = args.at(2, nullptr);

    if (!message && kwargs && !kwargs->is_empty())
        message = kwargs;
    else
        env->ensure_no_extra_keywords(kwargs);

    if (!klass && !message && cause)
        env->raise("ArgumentError", "only cause is given with no arguments");

    if (klass && klass->is_class() && !message)
        return _new(env, klass->as_class(), {}, nullptr)->as_exception_or_raise(env);

    if (!klass && current_exception)
        klass = current_exception;

    if (!klass) {
        klass = find_top_level_const(env, "RuntimeError"_s);
        message = new StringObject { "" };
    }

    if (!message) {
        Value arg = klass;
        if (arg->is_string()) {
            klass = find_top_level_const(env, "RuntimeError"_s)->as_class();
            message = arg;
        } else if (arg->is_exception()) {
            return arg->as_exception();
        } else if (!arg->is_class()) {
            env->raise("TypeError", "exception klass/object expected");
        }
    }

    Vector<Value> exception_args;
    if (message)
        exception_args.push(message);

    ExceptionObject *exception;
    if (klass->is_class())
        exception = _new(env, klass->as_class(), { std::move(exception_args), false }, nullptr)->as_exception();
    else if (klass->is_exception())
        exception = Value(klass).send(env, "exception"_s, { std::move(exception_args), false })->as_exception_or_raise(env);
    else
        env->raise("TypeError", "exception klass/object expected");

    if (accept_cause && cause && cause->is_exception()) {
        exception->set_cause(cause->as_exception());
    }

    if (backtrace)
        exception->set_backtrace(env, backtrace);

    return exception;
}

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

bool ExceptionObject::eq(Env *env, Value other) {
    if (!other->is_exception()) return false;
    auto exc = other->as_exception();
    return m_klass == exc->m_klass && message(env)->send(env, "=="_s, { exc->message(env) })->is_truthy() && backtrace(env)->send(env, "=="_s, { exc->backtrace(env) })->is_truthy();
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
    if (!m_backtrace && !m_backtrace_value)
        return NilObject::the();
    if (!m_backtrace_value)
        m_backtrace_value = m_backtrace->to_ruby_array();
    return m_backtrace_value;
}

Value ExceptionObject::backtrace_locations() {
    if (!m_backtrace)
        return NilObject::the();
    if (!m_backtrace_locations)
        m_backtrace_locations = m_backtrace->to_ruby_backtrace_locations_array();
    return m_backtrace_locations;
}

Value ExceptionObject::set_backtrace(Env *env, Value backtrace) {
    if (backtrace->is_array()) {
        for (auto element : *backtrace->as_array()) {
            if (!element->is_string())
                env->raise("TypeError", "backtrace must be Array of String");
        }
        m_backtrace_value = backtrace;
    } else if (backtrace->is_string()) {
        m_backtrace_value = new ArrayObject { backtrace };
    } else if (backtrace->is_nil()) {
        m_backtrace_value = nullptr;
    } else {
        env->raise("TypeError", "backtrace must be Array of String");
    }
    return m_backtrace_value;
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
    visitor.visit(m_backtrace_value);
    visitor.visit(m_backtrace_locations);
    visitor.visit(m_cause);
}

}
