#pragma once

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>

#include "minicoro.h"

#include "natalie/array_object.hpp"
#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/nil_object.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

extern "C" {
void fiber_wrapper_func(mco_coro *co);

typedef struct {
    Natalie::Env *env;
    Natalie::FiberObject *fiber;
} coroutine_user_data;
}

namespace Natalie {

class FiberObject : public Object {
public:
    enum class Status {
        Created,
        Resumed,
        Suspended,
        Terminated,
    };

    FiberObject()
        : Object { Object::Type::Fiber, GlobalEnv::the()->Object()->const_fetch("Fiber"_s)->as_class() } {
        add_to_list(this);
    }

    FiberObject(ClassObject *klass)
        : Object { Object::Type::Fiber, klass } {
        add_to_list(this);
    }

    ~FiberObject() {
        remove_from_list(this);
        if (!m_coroutine) {
            // FiberObject::initialize() can raise before the m_coroutine is created.
            return;
        }
        auto user_data = (coroutine_user_data *)m_coroutine->user_data;
        delete user_data;
        mco_destroy(m_coroutine);
    }

    static FiberObject *build_main_fiber(ThreadObject *, void *);

    constexpr static int STACK_SIZE = 1024 * 1024;

    FiberObject *initialize(Env *env, Value, Value, Block *block);

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
    void swap_to_previous(Env *env, Args args);

    void *start_of_stack() { return m_start_of_stack; }
    void *end_of_stack() { return m_end_of_stack; }
    void set_end_of_stack(void *ptr) { m_end_of_stack = ptr; }

    mco_coro *coroutine() { return m_coroutine; }
    Block *block() { return m_block; }

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

    virtual void gc_inspect(char *buf, size_t len) const override {
        auto size = m_coroutine ? m_coroutine->stack_size : 0;
        snprintf(
            buf,
            len,
            "<FiberObject %p stack=%p..%p>",
            this,
            m_end_of_stack,
            m_start_of_stack);
    }

    static FiberObject *current();
    static FiberObject *main();

    bool is_current() const;
    bool is_main() const;

    // This must be called with a GC reader lock already in place!
    static void each_fiber_for_gc(std::function<void(FiberObject *)> callback) {
        for (size_t i = 0; i < s_list_size; i++)
            callback(s_list[i]);
    }

    static void add_to_list(FiberObject *fiber) {
        GC_call_with_reader_lock(add_to_fiber_list, (void *)fiber, 1);
    }

    static void remove_from_list(FiberObject *fiber) {
        GC_call_with_reader_lock(remove_from_fiber_list, (void *)fiber, 1);
    }

    Vector<Value> &args() { return m_args; }
    void set_args(Args args);

    ExceptionObject *error() { return m_error; }
    void set_error(ExceptionObject *error) { m_error = error; }

    HashObject *thread_storage() { return m_thread_storage; }
    HashObject *ensure_thread_storage();

private:
    static void *add_to_fiber_list(void *info);
    static void *remove_from_fiber_list(void *info);

    Block *m_block { nullptr };
    bool m_blocking { false };
    HashObject *m_storage { nullptr };
    mco_coro *m_coroutine {};
    void *m_start_of_stack { nullptr };
    void *m_end_of_stack { nullptr };
    ThreadObject *m_thread { nullptr };
    HashObject *m_thread_storage { nullptr };
#ifdef __SANITIZE_ADDRESS__
    void *m_asan_fake_stack { nullptr };
#endif
    Status m_status { Status::Created };
    TM::Optional<TM::String> m_file {};
    TM::Optional<size_t> m_line {};
    Vector<Value> m_args {};
    FiberObject *m_previous_fiber { nullptr };
    ExceptionObject *m_error { nullptr };

    inline static FiberObject **s_list { nullptr };
    inline static size_t s_list_size { 0 };
};

}
