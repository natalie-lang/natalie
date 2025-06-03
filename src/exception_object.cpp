#include "natalie.hpp"

namespace Natalie {

ExceptionObject *ExceptionObject::create_for_raise(Env *env, Args &&args, ExceptionObject *current_exception, bool accept_cause) {
    auto kwargs = args.pop_keyword_hash();
    auto cause = accept_cause && kwargs ? kwargs->remove(env, "cause"_s) : Optional<Value>();
    args.ensure_argc_between(env, 0, 3);
    auto klass = args.maybe_at(0);
    auto message = args.maybe_at(1);
    auto backtrace = args.maybe_at(2);

    if (!message && kwargs && !kwargs->is_empty())
        message = kwargs;
    else
        env->ensure_no_extra_keywords(kwargs);

    if (!klass && !message && cause)
        env->raise("ArgumentError", "only cause is given with no arguments");

    if (klass && klass.value().is_class() && !message)
        return _new(env, klass.value().as_class(), {}, nullptr).as_exception_or_raise(env);

    if (klass && !klass.value().is_class() && klass.value().respond_to(env, "exception"_s)) {
        Vector<Value> args;
        if (message) args.push(message.value());
        klass = klass.value().send(env, "exception"_s, std::move(args));
    }

    if (!klass && current_exception)
        klass = current_exception;

    if (!klass) {
        klass = find_top_level_const(env, "RuntimeError"_s);
        message = StringObject::create("");
    }

    if (!message) {
        Value arg = klass.value();
        if (arg.is_string()) {
            klass = find_top_level_const(env, "RuntimeError"_s).as_class();
            message = arg;
        } else if (arg.is_exception()) {
            return arg.as_exception();
        } else if (!arg.is_class()) {
            env->raise("TypeError", "exception class/object expected");
        }
    }

    Vector<Value> exception_args;
    if (message)
        exception_args.push(message.value());

    ExceptionObject *exception;
    if (klass.value().is_class()) {
        auto possible_exception = _new(env, klass.value().as_class(), { std::move(exception_args), false }, nullptr);
        if (!possible_exception.is_exception())
            env->raise("TypeError", "exception object expected");
        exception = possible_exception.as_exception();
    } else if (klass.value().is_exception())
        exception = klass.value().as_exception();
    else
        env->raise("TypeError", "exception class/object expected");

    if (accept_cause && cause && cause.value().is_exception())
        exception->set_cause(cause.value().as_exception());

    if (backtrace)
        exception->set_backtrace(env, backtrace.value());

    return exception;
}

Value ExceptionObject::initialize(Env *env, Optional<Value> message) {
    if (message)
        set_message(message.value());
    return this;
}

// static exception
Value ExceptionObject::exception(Env *env, ClassObject *klass, Optional<Value> message) {
    ExceptionObject *exc = nullptr;
    if (klass)
        exc = ExceptionObject::create(klass);
    else
        exc = ExceptionObject::create();
    return exc->initialize(env, message);
}

Value ExceptionObject::exception(Env *env, Optional<Value> val) {
    if (!val) return this;
    if (val.value() == this) return this;
    auto exc = clone(env).as_exception();
    exc->set_message(val.value());
    return exc;
}

bool ExceptionObject::eq(Env *env, Value other) {
    if (!other.is_exception()) return false;
    auto exc = other.as_exception();
    return m_klass == exc->m_klass && message(env).send(env, "=="_s, { exc->message(env) }).is_truthy() && backtrace(env).send(env, "=="_s, { exc->backtrace(env) }).is_truthy();
}

Value ExceptionObject::inspect(Env *env) {
    auto klassname = m_klass->inspect_module();
    auto msgstr = send(env, "to_s"_s);
    msgstr.assert_type(env, Object::Type::String, "String");
    if (msgstr.as_string()->is_empty())
        return StringObject::create(klassname);
    if (msgstr.as_string()->include(env, StringObject::create("\n")))
        return StringObject::format("#<{}: {}>", klassname, klassname);
    return StringObject::format("#<{}: {}>", klassname, msgstr);
}

StringObject *ExceptionObject::to_s(Env *env) {
    if (m_message.is_nil()) {
        return StringObject::create(m_klass->inspect_module());
    } else if (m_message.is_string()) {
        return m_message.as_string();
    }
    auto msgstr = m_message.send(env, "to_s"_s);
    msgstr.assert_type(env, Object::Type::String, "String");
    return msgstr.as_string();
}

Value ExceptionObject::message(Env *env) {
    return send(env, "to_s"_s);
}

Value ExceptionObject::detailed_message(Env *env, Args &&args) {
    auto kwargs = args.pop_keyword_hash();
    const auto highlight = kwargs ? kwargs->delete_key(env, "highlight"_s, nullptr).is_truthy() : false;
    args.ensure_argc_is(env, 0);

    auto message = send(env, "message"_s).as_string();
    if (message->is_empty()) {
        if (klass() == find_top_level_const(env, "RuntimeError"_s).as_class()) {
            message->set_str("unhandled exception");
        } else {
            message->set_str(klass()->inspect_module().c_str());
        }
        if (highlight)
            message = StringObject::format("\e[1;4m{}\e[m", message);
        return message;
    }

    if (!klass()->name())
        return message;

    const char *fmt = highlight ? "\e[1m{} (\e[1;4m{}\e[m\e[1m)\e[m" : "{} ({})";
    return StringObject::format(fmt, message, klass()->inspect_module());
}

Value ExceptionObject::backtrace(Env *env) {
    if (!m_backtrace && !m_backtrace_value)
        return Value::nil();
    if (!m_backtrace_value)
        m_backtrace_value = m_backtrace->to_ruby_array();
    return m_backtrace_value;
}

Value ExceptionObject::backtrace_locations() {
    if (!m_backtrace)
        return Value::nil();
    if (!m_backtrace_locations)
        m_backtrace_locations = m_backtrace->to_ruby_backtrace_locations_array();
    return m_backtrace_locations;
}

Value ExceptionObject::set_backtrace(Env *env, Value backtrace) {
    if (backtrace.is_array()) {
        for (auto element : *backtrace.as_array()) {
            if (!element.is_string())
                env->raise("TypeError", "backtrace must be Array of String");
        }
        m_backtrace_value = backtrace.as_array();
    } else if (backtrace.is_string()) {
        m_backtrace_value = ArrayObject::create({ backtrace });
    } else if (backtrace.is_nil()) {
        m_backtrace_value = nullptr;
        return Value::nil();
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
    for (auto klass : *ary.as_array()) {
        if (klass.is_class()) {
            auto name = klass.as_class()->name();
            if (name && name.value() == "LocalJumpError")
                match_local_jump_error = true;
        }
    }
    if (!match_local_jump_error && m_local_jump_error_type != LocalJumpErrorType::None)
        return Value::False();

    for (auto klass : *ary.as_array()) {
        if (klass.send(env, "==="_s, { this }).is_truthy())
            return Value::True();
    }
    return Value::False();
}

bool ExceptionObject::is_local_jump_error_with_break_point(nat_int_t match_break_point) const {
    if (m_local_jump_error_type == LocalJumpErrorType::None) return false;
    if (m_break_point == 0) return false;
    return m_break_point == match_break_point;
}

void ExceptionObject::visit_children(Visitor &visitor) const {
    Object::visit_children(visitor);
    visitor.visit(m_message);
    visitor.visit(m_backtrace);
    visitor.visit(m_backtrace_value);
    visitor.visit(m_backtrace_locations);
    visitor.visit(m_cause);
}

}
