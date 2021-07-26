#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class ExceptionValue : public Value {
public:
    ExceptionValue()
        : Value { Value::Type::Exception, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Exception"))->as_class() } { }

    ExceptionValue(ClassValue *klass)
        : Value { Value::Type::Exception, klass } { }

    ExceptionValue(ClassValue *klass, StringValue *message)
        : Value { Value::Type::Exception, klass }
        , m_message { message } {
        assert(m_message);
    }

    ExceptionValue(Env *env, ExceptionValue &other)
        : Value { other } {
        m_message = other.m_message;
        m_backtrace = other.m_backtrace;
    }

    StringValue *message() { return m_message; }
    void set_message(StringValue *message) { m_message = message; }

    ValuePtr initialize(Env *, ValuePtr);
    ValuePtr inspect(Env *);

    ValuePtr message(Env *env) {
        return m_message;
    }

    const ArrayValue *backtrace() { return m_backtrace; }
    ValuePtr backtrace(Env *);
    void build_backtrace(Env *);

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) override {
        snprintf(buf, len, "<ExceptionValue %p message='%s'>", this, m_message->c_str());
    }

    void set_local_jump_error_env(Env *env) { m_local_jump_error_env = env; }
    Env *local_jump_error_env() { return m_local_jump_error_env; }

    void set_local_jump_error_type(LocalJumpErrorType type) { m_local_jump_error_type = type; }
    LocalJumpErrorType local_jump_error_type() { return m_local_jump_error_type; }

private:
    StringValue *m_message { nullptr };
    ArrayValue *m_backtrace { nullptr };
    Env *m_local_jump_error_env { nullptr };
    LocalJumpErrorType m_local_jump_error_type { LocalJumpErrorType::None };
};

}
