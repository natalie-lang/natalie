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
    static ExceptionObject *create() {
        return new ExceptionObject();
    }

    static ExceptionObject *create(ClassObject *klass) {
        return new ExceptionObject(klass);
    }

    static ExceptionObject *create(ClassObject *klass, Value message) {
        return new ExceptionObject(klass, message);
    }

    static ExceptionObject *create(const ExceptionObject &other) {
        return new ExceptionObject(other);
    }

    static ExceptionObject *create_for_raise(Env *env, Args &&args, ExceptionObject *current_exception, bool accept_cause);

    void set_message(Value message) { m_message = message; }

    static Value exception(Env *, ClassObject *klass, Optional<Value>);
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

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<ExceptionObject {h} message={}>", this, m_message.dbg_inspect(indent));
    }

    void set_local_jump_error_type(LocalJumpErrorType type) { m_local_jump_error_type = type; }
    LocalJumpErrorType local_jump_error_type() { return m_local_jump_error_type; }

    Value match_rescue_array(Env *env, Value ary);
    bool is_local_jump_error_with_break_point(nat_int_t match_break_point) const;

    nat_int_t break_point() const { return m_break_point; }
    void set_break_point(nat_int_t break_point) { m_break_point = break_point; }

private:
    ExceptionObject()
        : Object { Object::Type::Exception, GlobalEnv::the()->Object()->const_fetch("Exception"_s).as_class() } { }

    ExceptionObject(ClassObject *klass)
        : Object { Object::Type::Exception, klass } { }

    ExceptionObject(ClassObject *klass, Value message)
        : Object { Object::Type::Exception, klass }
        , m_message { message } { }

    ExceptionObject(const ExceptionObject &other)
        : Object { other }
        , m_message { other.m_message }
        , m_backtrace { other.m_backtrace }
        , m_backtrace_value { other.m_backtrace_value }
        , m_backtrace_locations { other.m_backtrace_locations }
        , m_cause { other.m_cause } { }

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
