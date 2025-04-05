#pragma once

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>

#include "minicoro/minicoro.h"

#include "natalie/array_object.hpp"
#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie/thread_object.hpp"

extern "C" {
void fiber_wrapper_func(mco_coro *co);

struct coroutine_user_data {
    Natalie::Env *env;
    Natalie::FiberObject *fiber;
};
}

namespace Natalie {

extern thread_local Vector<Value> *tl_current_arg_stack;

class FiberObject : public Object {
public:
    enum class Status {
        Created,
        Resumed,
        Suspended,
        Terminated,
    };

    FiberObject()
        : Object { Object::Type::Fiber, GlobalEnv::the()->Object()->const_fetch("Fiber"_s).as_class() } { }

    FiberObject(ClassObject *klass)
        : Object { Object::Type::Fiber, klass } { }

    ~FiberObject() {
        if (!m_coroutine) {
            // FiberObject::initialize() can raise before the m_coroutine is created.
            return;
        }
        auto user_data = (coroutine_user_data *)m_coroutine->user_data;
        delete user_data;
        mco_destroy(m_coroutine);
    }

    constexpr static int STACK_SIZE = 1024 * 1024;

    FiberObject *initialize(Env *env, Optional<Value>, Optional<Value>, Block *block);

    static Value yield(Env *env, Args args);

    Value hash(Env *);
    Value inspect(Env *);
    bool is_alive() const;
    bool is_blocking() const;
    static Value blocking(Env *, Block *);
    static Value is_blocking_current();
    static Value ref(Env *env, Value);
    static Value refeq(Env *env, Value, Value);
    Value resume(Env *env, Args args);
    static Value scheduler();
    static bool scheduler_is_relevant();
    static Value set_scheduler(Env *, Value);
    Value set_storage(Env *, Value);
    Value storage(Env *) const;
    void swap_to_previous(Env *env, size_t arg_size, Value *arg_data);

    mco_coro *coroutine() { return m_coroutine; }
    Block *block() { return m_block; }

    // "bottom" -- high address
    void *start_of_stack() { return m_start_of_stack; }
    void set_start_of_stack(void *ptr) { m_start_of_stack = ptr; }

    // "top" -- low address
    void *end_of_stack() { return m_end_of_stack; }
    void set_end_of_stack(void *ptr) { m_end_of_stack = ptr; }

    void set_status(Status status) { m_status = status; }
    SymbolObject *status(Env *env) {
        switch (m_status) {
        case Status::Created:
            return "created"_s;
        case Status::Resumed:
            return "resumed"_s;
        case Status::Suspended:
            return "suspended"_s;
        case Status::Terminated:
            return "terminated"_s;
        }
        NAT_UNREACHABLE();
    }

    virtual void visit_children(Visitor &) const override final;
    void visit_children_from_stack(Visitor &) const;
    void visit_children_from_asan_fake_stack(Visitor &, Cell *) const;

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<FiberObject {h} stack={h}..{h}>", this, m_end_of_stack, m_start_of_stack);
    }

    static FiberObject *current() { return tl_current_thread->current_fiber(); }
    static FiberObject *main() { return tl_current_thread->main_fiber(); }

    Vector<Value> &args() { return m_args; }
    void set_args(size_t arg_size, Value *arg_data);

    ExceptionObject *error() { return m_error; }
    void set_error(ExceptionObject *error) { m_error = error; }

    HashObject *thread_storage() { return m_thread_storage; }
    HashObject *ensure_thread_storage();

    FiberObject *previous_fiber() const { return m_previous_fiber; }

    void set_redo_block() { m_redo_block = true; }
    bool check_redo_block_and_clear() {
        auto value = m_redo_block;
        m_redo_block = false;
        return value;
    }

#ifdef __SANITIZE_ADDRESS__
    void *
    asan_stack_base() const {
        return m_asan_stack_base;
    }
    void set_asan_stack_base(void *base) { m_asan_stack_base = base; }

    size_t asan_stack_size() const { return m_asan_stack_size; }
    void set_asan_stack_size(size_t size) { m_asan_stack_size = size; }

    void *asan_fake_stack_start() const { return m_asan_fake_stack_start; }
    void set_asan_fake_stack_start(void *start) { m_asan_fake_stack_start = start; }
#endif

private:
    friend Args;
    friend ThreadObject;

    TM::Vector<Value> m_args_stack { 100 };
    Block *m_block { nullptr };
    bool m_blocking { false };
    HashObject *m_storage { nullptr };
    mco_coro *m_coroutine {};
    void *m_start_of_stack { nullptr };
    void *m_end_of_stack { nullptr };
    ThreadObject *m_thread { nullptr };
    HashObject *m_thread_storage { nullptr };
#ifdef __SANITIZE_ADDRESS__
    void *m_asan_fake_stack_start { nullptr };
    void *m_asan_stack_base { nullptr };
    size_t m_asan_stack_size { 0 };
#endif
    Status m_status { Status::Created };
    TM::Optional<TM::String> m_file {};
    TM::Optional<size_t> m_line {};
    TM::Vector<Value> m_args {};
    FiberObject *m_previous_fiber { nullptr };
    ExceptionObject *m_error { nullptr };
    bool m_redo_block { false };
};

}
