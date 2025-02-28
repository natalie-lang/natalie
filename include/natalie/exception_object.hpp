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
        : Object { Object::Type::Exception, GlobalEnv::the()->Object()->const_fetch("Exception"_s).as_class() } { }

    ExceptionObject(ClassObject *klass)
        : Object { Object::Type::Exception, klass } { }

    ExceptionObject(ClassObject *klass, Value message)
        : Object { Object::Type::Exception, klass }
        , m_message { message } { }

    ExceptionObject(Env *env, ExceptionObject &other)
        : Object { other } {
        m_message = other.m_message;
        m_backtrace = other.m_backtrace;
    }

    static ExceptionObject *create_for_raise(Env *env, Args &&args, ExceptionObject *current_exception, bool accept_cause);

    void set_message(Value message) { m_message = message; }

    static Value exception(Env *, Optional<Value>, Optional<ClassObject *> klass);
    Value exception(Env *, Optional<Value>);
    Value initialize(Env *, Optional<Value>);
    bool eq(Env *, Value);
    Value inspect(Env *);

    StringObject *to_s(Env *env);
    Value message(Env *env);
    Value detailed_message(Env *, Args &&);

    Backtrace *backtrace() { return m_backtrace; }
    void build_backtrace(Env *env) { m_backtrace = env->backtrace(); }
    Value backtrace(Env *);
    Value backtrace_locations();
    Value set_backtrace(Env *, Value);

    ExceptionObject *cause() const { return m_cause; }
    void set_cause(ExceptionObject *e) { m_cause = e; }

    virtual void visit_children(Visitor &) const override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<ExceptionObject %p message=?>", this);
    }

    void set_local_jump_error_type(LocalJumpErrorType type) { m_local_jump_error_type = type; }
    LocalJumpErrorType local_jump_error_type() { return m_local_jump_error_type; }

    Value match_rescue_array(Env *env, Value ary);
    bool is_local_jump_error_with_break_point(nat_int_t match_break_point) const;

    nat_int_t break_point() const { return m_break_point; }
    void set_break_point(nat_int_t break_point) { m_break_point = break_point; }

private:
    ArrayObject *generate_backtrace();

    Value m_message { Value::nil() };
    Backtrace *m_backtrace { nullptr };
    ArrayObject *m_backtrace_value { nullptr };
    ArrayObject *m_backtrace_locations { nullptr };
    ExceptionObject *m_cause { nullptr };
    nat_int_t m_break_point { 0 };
    LocalJumpErrorType m_local_jump_error_type { LocalJumpErrorType::None };
};

}
