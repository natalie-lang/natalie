#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class ExceptionObject : public Object {
public:
    ExceptionObject()
        : Object { Object::Type::Exception, GlobalEnv::the()->Object()->const_fetch("Exception"_s)->as_class() } { }

    ExceptionObject(ClassObject *klass)
        : Object { Object::Type::Exception, klass } { }

    ExceptionObject(ClassObject *klass, StringObject *message)
        : Object { Object::Type::Exception, klass }
        , m_message { message } {
        assert(m_message);
    }

    ExceptionObject(Env *env, ExceptionObject &other)
        : Object { other } {
        m_message = other.m_message;
        m_backtrace = other.m_backtrace;
    }

    StringObject *message() { return m_message; }
    void set_message(StringObject *message) { m_message = message; }

    Value initialize(Env *, Value);
    Value inspect(Env *);

    Value message(Env *env) {
        return m_message;
    }

    const ArrayObject *backtrace() { return m_backtrace; }
    void build_backtrace(Env *env) { m_backtrace = env->backtrace(); }
    Value backtrace(Env *);

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<ExceptionObject %p message='%s'>", this, m_message->c_str());
    }

    void set_local_jump_error_env(Env *env) { m_local_jump_error_env = env; }
    Env *local_jump_error_env() { return m_local_jump_error_env; }

    void set_local_jump_error_type(LocalJumpErrorType type) { m_local_jump_error_type = type; }
    LocalJumpErrorType local_jump_error_type() { return m_local_jump_error_type; }

    Value match_rescue_array(Env *env, Value ary);

private:
    StringObject *m_message { nullptr };
    ArrayObject *m_backtrace { nullptr };
    Env *m_local_jump_error_env { nullptr };
    LocalJumpErrorType m_local_jump_error_type { LocalJumpErrorType::None };
};

}
