#include "natalie.hpp"

#define MINICORO_IMPL
#include "minicoro/minicoro.h"

#ifdef __SANITIZE_ADDRESS__
extern "C" void __sanitizer_start_switch_fiber(void **fake_stack_save, const void *bottom, size_t size);
extern "C" void __sanitizer_finish_switch_fiber(void *fake_stack_save, const void **bottom_old, size_t *size_old);
#endif

namespace Natalie {

#ifdef __SANITIZE_ADDRESS__
#define START_SWITCH_FIBER(from_fiber, to_fiber)                                                                     \
    {                                                                                                                \
        void *fake_stack_start = nullptr;                                                                            \
        __sanitizer_start_switch_fiber(&fake_stack_start, to_fiber->asan_stack_base(), to_fiber->asan_stack_size()); \
        from_fiber->set_asan_fake_stack_start(fake_stack_start);                                                     \
    }

#define START_SWITCH_FIBER_FINAL(to_fiber)                                                                 \
    {                                                                                                      \
        __sanitizer_start_switch_fiber(nullptr, to_fiber->asan_stack_base(), to_fiber->asan_stack_size()); \
    }

#define FINISH_SWITCH_FIBER(from_fiber, to_fiber)                                                                    \
    {                                                                                                                \
        void *stack_base = nullptr;                                                                                  \
        size_t stack_size = 0;                                                                                       \
        __sanitizer_finish_switch_fiber(to_fiber->asan_fake_stack_start(), (const void **)&stack_base, &stack_size); \
        from_fiber->set_asan_stack_base(stack_base);                                                                 \
        from_fiber->set_asan_stack_size(stack_size);                                                                 \
    }
#else
#define START_SWITCH_FIBER(from_fiber, to_fiber)
#define START_SWITCH_FIBER_FINAL(to_fiber)
#define FINISH_SWITCH_FIBER(from_fiber, to_fiber)
#endif

thread_local TM::Vector<Value> *tl_current_arg_stack = nullptr;

FiberObject *FiberObject::initialize(Env *env, Optional<Value> blocking_kwarg, Optional<Value> storage_kwarg, Block *block) {
    assert(this != FiberObject::main()); // can never be main fiber

    env->ensure_block_given(block);
    m_block = block;

    if (blocking_kwarg)
        m_blocking = blocking_kwarg->is_truthy();

    m_file = env->file();
    m_line = env->line();

    if (storage_kwarg && !storage_kwarg->is_nil()) {
        auto storage = storage_kwarg.value();
        if (!storage.is_hash())
            env->raise("TypeError", "storage must be a hash");
        if (storage->is_frozen())
            env->raise("FrozenError", "storage must not be frozen");
        auto *hash = storage.as_hash();
        for (auto it = hash->begin(); it != hash->end(); it++) {
            if (!it->key.is_symbol())
                env->raise("TypeError", "wrong argument type Object (expected Symbol)");
        }
        m_storage = hash;
    }

    m_thread = ThreadObject::current();

    mco_desc desc = mco_desc_init(fiber_wrapper_func, STACK_SIZE);
    desc.user_data = new coroutine_user_data { env, this };
    mco_result res = mco_create(&m_coroutine, &desc);
    assert(res == MCO_SUCCESS);
    m_start_of_stack = (void *)((uintptr_t)m_coroutine->stack_base + m_coroutine->stack_size);
    m_end_of_stack = (void *)((uintptr_t)m_start_of_stack - STACK_SIZE);

#ifdef __SANITIZE_ADDRESS__
    m_asan_stack_base = m_start_of_stack;
    m_asan_stack_size = STACK_SIZE;
#endif

    return this;
}

Value FiberObject::hash(Env *env) {
    const TM::String file_and_line { m_file && m_line ? TM::String::format(" {}:{}", *m_file, *m_line) : "" };
    const auto hash = HashKeyHandler<String>::hash(String::format("{}{}{}", m_klass->inspect_module(), String::hex(object_id(this), String::HexFormat::LowercaseAndPrefixed), file_and_line));
    return Value::integer(hash);
}

Value FiberObject::inspect(Env *env) {
    const TM::String file_and_line { m_file && m_line ? TM::String::format(" {}:{}", *m_file, *m_line) : "" };
    return StringObject::format("#<{}:{}{} ({})>", m_klass->inspect_module(), String::hex(object_id(this), String::HexFormat::LowercaseAndPrefixed), file_and_line, status(env));
}

bool FiberObject::is_alive() const {
    return m_status != Status::Terminated;
}

bool FiberObject::is_blocking() const {
    return m_blocking;
}

Value FiberObject::blocking(Env *env, Block *block) {
    auto fiber = current();
    if (fiber->is_blocking()) {
        return block->run(env, { fiber }, nullptr);
    }
    fiber->m_blocking = true;
    auto unblock = Defer([&fiber] { fiber->m_blocking = false; });
    return block->run(env, { fiber }, nullptr);
}

Value FiberObject::is_blocking_current() {
    return current()->is_blocking() ? Value::integer(1) : Value::False();
}

Value FiberObject::ref(Env *env, Value key) {
    const static auto to_str = "to_str"_s;
    if (key.is_string() || key.respond_to(env, to_str))
        key = key.to_str(env)->to_sym(env);
    if (!key.is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key.klass()->inspect_module());
    auto fiber = current();
    while ((fiber->m_storage == nullptr || !fiber->m_storage->has_key(env, key)) && fiber->m_previous_fiber != nullptr)
        fiber = fiber->m_previous_fiber;
    if (fiber->m_storage == nullptr)
        return Value::nil();
    return fiber->m_storage->ref(env, key);
}

Value FiberObject::refeq(Env *env, Value key, Value value) {
    const static auto to_str = "to_str"_s;
    if (key.is_string() || key.respond_to(env, to_str))
        key = key.to_str(env)->to_sym(env);
    if (!key.is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key.klass()->inspect_module());
    if (current()->m_storage == nullptr)
        current()->m_storage = HashObject::create();
    if (value.is_nil()) {
        current()->m_storage->remove(env, key);
    } else {
        current()->m_storage->refeq(env, key, value);
    }
    return value;
}

NO_SANITIZE_ADDRESS Value FiberObject::resume(Env *env, Args args) {
    if (m_thread != ThreadObject::current())
        env->raise("FiberError", "fiber called across threads");
    if (m_status == Status::Terminated)
        env->raise("FiberError", "dead fiber called");
    if (m_previous_fiber)
        env->raise("FiberError", "attempt to resume the current fiber");
    if (this == FiberObject::main())
        env->raise("FiberError", "attempt to resume a resuming fiber");

    auto suspending_fiber = m_previous_fiber = current();

    // RESUME START SWITCH
    START_SWITCH_FIBER(suspending_fiber, this);

    {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        set_args(args.size(), args.data());
        ThreadObject::current()->m_current_fiber = this;
        tl_current_arg_stack = &m_args_stack;
        ThreadObject::current()->set_start_of_stack(m_start_of_stack);
        void *dummy;
        suspending_fiber->m_end_of_stack = &dummy;
    }

    auto res = mco_resume(m_coroutine);
    assert(res == MCO_SUCCESS);

    auto new_current = FiberObject::current();

    // YIELD FINISH SWITCH
    FINISH_SWITCH_FIBER(this, new_current);

    if (m_error)
        env->raise_exception(m_error);

    auto fiber_args = new_current->args();
    if (fiber_args.size() == 0) {
        return Value::nil();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return ArrayObject::create(fiber_args.size(), fiber_args.data());
    }
}

Value FiberObject::scheduler() {
    return ThreadObject::current()->fiber_scheduler();
}

bool FiberObject::scheduler_is_relevant() {
    if (FiberObject::current()->is_blocking())
        return false;

    auto scheduler = FiberObject::scheduler();
    return !scheduler.is_nil();
}

Value FiberObject::set_scheduler(Env *env, Value scheduler) {
    if (scheduler.is_nil()) {
        ThreadObject::current()->set_fiber_scheduler(Value::nil());
    } else {
        TM::Vector<TM::String> required_methods { "block", "unblock", "kernel_sleep", "io_wait" };
        for (const auto &required_method : required_methods) {
            if (!scheduler.respond_to(env, SymbolObject::intern(required_method)))
                env->raise("ArgumentError", "Scheduler must implement #{}", required_method);
        }
        ThreadObject::current()->set_fiber_scheduler(scheduler);
    }
    return scheduler;
}

Value FiberObject::set_storage(Env *env, Value storage) {
    if (storage == nullptr || storage.is_nil()) {
        m_storage = nullptr;
        return Value::nil();
    } else if (!storage.is_hash()) {
        env->raise("TypeError", "storage must be a hash");
    } else {
        if (storage->is_frozen())
            env->raise("FrozenError", "storage must not be frozen");
        auto *hash = storage.as_hash();
        for (auto it = hash->begin(); it != hash->end(); it++) {
            if (!it->key.is_symbol())
                env->raise("TypeError", "wrong argument type Object (expected Symbol)");
        }
        m_storage = hash;
    }
    return m_storage;
}

Value FiberObject::storage(Env *env) const {
    auto fiber = FiberObject::current();
    if (this != fiber)
        env->raise("ArgumentError", "Fiber storage can only be accessed from the Fiber it belongs to");
    while (fiber->m_storage == nullptr && fiber->m_previous_fiber != nullptr)
        fiber = fiber->m_previous_fiber;
    if (fiber->m_storage == nullptr)
        return Value::nil();
    return fiber->m_storage;
}

NO_SANITIZE_ADDRESS Value FiberObject::yield(Env *env, Args args) {
    auto current_fiber = FiberObject::current();
    auto previous_fiber = current_fiber->m_previous_fiber;
    if (!previous_fiber)
        env->raise("FiberError", "can't yield from root fiber");

    {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        current_fiber->set_status(Status::Suspended);
        void *dummy;
        current_fiber->m_end_of_stack = &dummy;
        current_fiber->swap_to_previous(env, args.size(), args.data());
    }

    // YIELD START SWITCH
    START_SWITCH_FIBER(current_fiber, previous_fiber);

    mco_yield(mco_running());

    auto new_current = FiberObject::current();

    // RESUME FINISH SWITCH
    FINISH_SWITCH_FIBER(new_current->previous_fiber(), new_current);

    auto fiber_args = new_current->args();
    if (fiber_args.size() == 0) {
        return Value::nil();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return ArrayObject::create(fiber_args.size(), fiber_args.data());
    }
}

void FiberObject::swap_to_previous(Env *env, size_t arg_size, Value *arg_data) {
    assert(m_previous_fiber);
    auto new_current = m_previous_fiber;
    {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        new_current->set_args(arg_size, arg_data);
        ThreadObject::current()->m_current_fiber = new_current;
        tl_current_arg_stack = &new_current->m_args_stack;
        ThreadObject::current()->set_start_of_stack(new_current->start_of_stack());
    }
    m_previous_fiber = nullptr;
}

void FiberObject::visit_children(Visitor &visitor) const {
    Object::visit_children(visitor);
    for (auto arg : m_args)
        visitor.visit(arg);
    for (auto arg : m_args_stack)
        visitor.visit(arg);
    visitor.visit(m_previous_fiber);
    visitor.visit(m_error);
    visitor.visit(m_block);
    visitor.visit(m_storage);
    visitor.visit(m_thread);
    visitor.visit(m_thread_storage);
    visit_children_from_stack(visitor);
}

NO_SANITIZE_ADDRESS void FiberObject::visit_children_from_stack(Visitor &visitor) const {
    // If this Fiber is the currently active Fiber for its parent Thread,
    // then the Thread#visit_children_from_stack method will walk this Fiber's stack,
    // and we can bail out here.
    if (m_thread && this == m_thread->current_fiber())
        return;

    // If the fiber is finished, we don't care about the stack any more.
    if (m_status == Status::Terminated)
        return;

#ifdef __SANITIZE_ADDRESS__
    Heap::the().scan_memory(visitor, m_end_of_stack, m_start_of_stack, [&](Cell *potential_cell) {
        visit_children_from_asan_fake_stack(visitor, potential_cell);
    });
#else
    Heap::the().scan_memory(visitor, m_end_of_stack, m_start_of_stack);
#endif
}

#ifdef __SANITIZE_ADDRESS__
NO_SANITIZE_ADDRESS void FiberObject::visit_children_from_asan_fake_stack(Visitor &visitor, Cell *potential_cell) const {
    void *begin = nullptr;
    void *end = nullptr;
    void *real_stack = __asan_addr_is_in_fake_stack(m_asan_fake_stack_start, potential_cell, &begin, &end);

    if (!real_stack) return;

    Heap::the().scan_memory(visitor, begin, end);
}
#else
void FiberObject::visit_children_from_asan_fake_stack(Visitor &visitor, Cell *potential_cell) const { }
#endif

void FiberObject::set_args(size_t arg_size, Value *arg_data) {
    m_args.clear();
    for (size_t i = 0; i < arg_size; ++i) {
        m_args.push(arg_data[i]);
    }
}

HashObject *FiberObject::ensure_thread_storage() {
    if (!m_thread_storage)
        m_thread_storage = HashObject::create();
    return m_thread_storage;
}
}

extern "C" {

void fiber_wrapper_func(mco_coro *co) {
    auto user_data = (coroutine_user_data *)co->user_data;
    auto env = user_data->env;
    auto fiber = user_data->fiber;
    auto previous_fiber = fiber->previous_fiber();
    (void)previous_fiber;

    // RESUME FINISH SWITCH
    FINISH_SWITCH_FIBER(previous_fiber, fiber);

    {
        std::lock_guard<std::recursive_mutex> lock(Natalie::g_gc_recursive_mutex);
        Natalie::ThreadObject::current()->set_start_of_stack(fiber->start_of_stack());
#ifdef __SANITIZE_ADDRESS__
        fiber->set_asan_fake_stack_start(__asan_get_current_fake_stack());
#endif
        fiber->set_status(Natalie::FiberObject::Status::Resumed);
    }

    assert(fiber->block());
    Natalie::Value return_arg;
    bool reraise = false;

    try {
        // NOTE: we cannot pass the env from this fiber (stack) across to another fiber (stack),
        // because doing so can cause unexpected results when env->caller() is used. (Since that
        // calling Env might have been overwritten in the other stack.)
        // That means a backtrace built from inside the fiber will end abruptly.
        // But that seems to be what Ruby does too.
        Natalie::Env e {};

        return_arg = fiber->block()->run(&e, Natalie::Args(fiber->args()), nullptr);
    } catch (Natalie::ExceptionObject *exception) {
        fiber->set_error(exception);
        reraise = true;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(Natalie::g_gc_recursive_mutex);
        fiber->set_status(Natalie::FiberObject::Status::Terminated);
        fiber->set_end_of_stack(&fiber);
    }

    if (reraise) {
        fiber->swap_to_previous(env, 0, nullptr);
    } else {
        fiber->swap_to_previous(env, 1, &return_arg);
    }

    // YIELD START SWITCH (FINAL IMPLICIT YIELD)
    //__sanitizer_start_switch_fiber(nullptr, previous_fiber->start_of_stack(), previous_fiber->stack_size());
    START_SWITCH_FIBER_FINAL(previous_fiber);
}
}
