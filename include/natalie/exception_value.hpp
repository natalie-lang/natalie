#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ExceptionValue : Value {
    ExceptionValue(Env *env)
        : Value { Value::Type::Exception, env->Object()->const_fetch(env, SymbolValue::intern(env, "Exception"))->as_class() } { }

    ExceptionValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Exception, klass } { }

    ExceptionValue(Env *env, ClassValue *klass, StringValue *message)
        : Value { Value::Type::Exception, klass }
        , m_message { message } {
        assert(m_message);
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

    virtual char *gc_repr() override {
        char *buf = new char[100];
        snprintf(buf, 100, "<ExceptionValue %p message='%s'>", this, m_message->c_str());
        return buf;
    }

private:
    StringValue *m_message { nullptr };
    ArrayValue *m_backtrace { nullptr };
};

}
