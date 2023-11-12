#include "natalie.hpp"

#define MINICORO_IMPL
#include "minicoro.h"

namespace Natalie {

FiberObject *FiberObject::initialize(Env *env, Value blocking, Value storage, Block *block) {
    assert(this != FiberObject::main()); // can never be main fiber

    env->ensure_block_given(block);
    m_block = block;

    if (blocking != nullptr)
        m_blocking = blocking->is_truthy();

    m_file = env->file();
    m_line = env->line();

    if (storage != nullptr && !storage->is_nil()) {
        if (!storage->is_hash())
            env->raise("TypeError", "storage must be a hash");
        if (storage->is_frozen())
            env->raise("FrozenError", "storage must not be frozen");
        auto *hash = storage->as_hash();
        for (auto it = hash->begin(); it != hash->end(); it++) {
            if (!it->key->is_symbol())
                env->raise("TypeError", "wrong argument type Object (expected Symbol)");
        }
        m_storage = hash;
    }

    mco_desc desc = mco_desc_init(fiber_wrapper_func, STACK_SIZE);
    desc.user_data = new coroutine_user_data { env, this };
    mco_result res = mco_create(&m_coroutine, &desc);
    assert(res == MCO_SUCCESS);
    m_start_of_stack = (void *)((uintptr_t)m_coroutine->stack_base + m_coroutine->stack_size);

    return this;
}

Value FiberObject::hash(Env *env) {
    const TM::String file_and_line { m_file && m_line ? TM::String::format(" {}:{}", *m_file, *m_line) : "" };
    const auto hash = String::format("{}{}{}", m_klass->inspect_str(), String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed), file_and_line).djb2_hash();
    return Value::integer(hash);
}

Value FiberObject::inspect(Env *env) {
    const TM::String file_and_line { m_file && m_line ? TM::String::format(" {}:{}", *m_file, *m_line) : "" };
    return StringObject::format("#<{}:{}{} ({})>", m_klass->inspect_str(), String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed), file_and_line, status(env));
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
        return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { fiber }, nullptr);
    }
    fiber->m_blocking = true;
    auto unblock = Defer([&fiber] { fiber->m_blocking = false; });
    return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { fiber }, nullptr);
}

Value FiberObject::is_blocking_current() {
    return s_current->is_blocking() ? IntegerObject::create(1) : FalseObject::the();
}

Value FiberObject::ref(Env *env, Value key) {
    if (!key->is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key->klass()->inspect_str());
    auto fiber = s_current;
    while ((fiber->m_storage == nullptr || !fiber->m_storage->has_key(env, key)) && fiber->m_previous_fiber != nullptr)
        fiber = fiber->m_previous_fiber;
    if (fiber->m_storage == nullptr)
        return NilObject::the();
    return fiber->m_storage->ref(env, key);
}

Value FiberObject::refeq(Env *env, Value key, Value value) {
    if (!key->is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key->klass()->inspect_str());
    if (s_current->m_storage == nullptr)
        s_current->m_storage = new HashObject {};
    s_current->m_storage->refeq(env, key, value);
    return value;
}

Value FiberObject::resume(Env *env, Args args) {
    if (m_status == Status::Terminated)
        env->raise("FiberError", "dead fiber called");
    if (m_previous_fiber)
        env->raise("FiberError", "attempt to resume the current fiber");

    auto suspending_fiber = s_current;
    m_previous_fiber = s_current;
    s_current = this;

    set_args(args);

    Heap::the().set_start_of_stack(m_start_of_stack);

#ifdef __SANITIZE_ADDRESS__
    auto fake_stack = __asan_get_current_fake_stack();
    void *real_stack = __asan_addr_is_in_fake_stack(fake_stack, &args, nullptr, nullptr);
    suspending_fiber->m_end_of_stack = real_stack ? real_stack : &args;
    suspending_fiber->m_asan_fake_stack = __asan_get_current_fake_stack();
#else
    suspending_fiber->m_end_of_stack = &args;
#endif

    auto res = mco_resume(m_coroutine);
    assert(res == MCO_SUCCESS);

    if (m_error)
        env->raise_exception(m_error);

    auto fiber_args = FiberObject::current()->args();
    if (fiber_args.size() == 0) {
        return NilObject::the();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return new ArrayObject { fiber_args.size(), fiber_args.data() };
    }
}

Value FiberObject::scheduler() {
    return s_scheduler;
}

bool FiberObject::scheduler_is_relevant() {
    return !FiberObject::current()->is_blocking() && FiberObject::scheduler() && !FiberObject::scheduler()->is_nil();
}

Value FiberObject::set_scheduler(Env *env, Value scheduler) {
    if (scheduler->is_nil()) {
        s_scheduler = nullptr;
    } else {
        TM::Vector<TM::String> required_methods { "block", "unblock", "kernel_sleep", "io_wait" };
        for (const auto &required_method : required_methods) {
            if (!scheduler->respond_to(env, SymbolObject::intern(required_method)))
                env->raise("ArgumentError", "Scheduler must implement #{}", required_method);
        }
        s_scheduler = scheduler;
    }
    return scheduler;
}

Value FiberObject::set_storage(Env *env, Value storage) {
    if (storage == nullptr || storage->is_nil()) {
        m_storage = nullptr;
    } else if (!storage->is_hash()) {
        env->raise("TypeError", "storage must be a hash");
    } else {
        if (storage->is_frozen())
            env->raise("FrozenError", "storage must not be frozen");
        auto *hash = storage->as_hash();
        for (auto it = hash->begin(); it != hash->end(); it++) {
            if (!it->key->is_symbol())
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
        return NilObject::the();
    return fiber->m_storage;
}

Value FiberObject::yield(Env *env, Args args) {
    auto current_fiber = FiberObject::current();
    if (!current_fiber->m_previous_fiber)
        env->raise("FiberError", "can't yield from root fiber");
    current_fiber->set_status(Status::Suspended);
    current_fiber->m_end_of_stack = &args;
    current_fiber->swap_current(env, args);

    mco_yield(mco_running());

    auto fiber_args
        = FiberObject::current()->args();
    if (fiber_args.size() == 0) {
        return NilObject::the();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return new ArrayObject { fiber_args.size(), fiber_args.data() };
    }
}

void FiberObject::swap_current(Env *env, Args args) {
    assert(m_previous_fiber);
    s_current = m_previous_fiber;
    s_current->set_args(args);
    m_previous_fiber = nullptr;
    Heap::the().set_start_of_stack(s_current->start_of_stack());
}

void FiberObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    for (auto arg : m_args)
        visitor.visit(arg);
    visitor.visit(m_previous_fiber);
    visitor.visit(m_error);
    visitor.visit(m_block);
    visitor.visit(m_storage);
    visit_children_from_stack(visitor);
}

NO_SANITIZE_ADDRESS void FiberObject::visit_children_from_stack(Visitor &visitor) const {
    if (m_start_of_stack == Heap::the().start_of_stack())
        return; // this is the currently active fiber, so don't walk its stack a second time
    if (!m_end_of_stack) {
        assert(m_status == Status::Created);
        return; // this fiber hasn't been started yet, so the stack shouldn't have anything on it
    }
    for (char *ptr = reinterpret_cast<char *>(m_end_of_stack); ptr < m_start_of_stack; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
        if (Heap::the().is_a_heap_cell_in_use(potential_cell))
            visitor.visit(potential_cell);
#ifdef __SANITIZE_ADDRESS__
        visit_children_from_asan_fake_stack(visitor, potential_cell);
#endif
    }
}

#ifdef __SANITIZE_ADDRESS__
NO_SANITIZE_ADDRESS void FiberObject::visit_children_from_asan_fake_stack(Visitor &visitor, Cell *potential_cell) const {
    void *begin = nullptr;
    void *end = nullptr;
    void *real_stack = __asan_addr_is_in_fake_stack(m_asan_fake_stack, potential_cell, &begin, &end);

    if (!real_stack) return;

    for (char *ptr = reinterpret_cast<char *>(begin); ptr < end; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
        if (Heap::the().is_a_heap_cell_in_use(potential_cell))
            visitor.visit(potential_cell);
    }
}
#else
void FiberObject::visit_children_from_asan_fake_stack(Visitor &visitor, Cell *potential_cell) const { }
#endif

void FiberObject::set_args(Args args) {
    m_args.clear();
    for (size_t i = 0; i < args.size(); ++i) {
        m_args.push(args[i]);
    }
}
}

extern "C" {

void fiber_wrapper_func(mco_coro *co) {
    auto user_data = (coroutine_user_data *)co->user_data;
    auto env = user_data->env;
    auto fiber = user_data->fiber;
    Natalie::Heap::the().set_start_of_stack(fiber->start_of_stack());
    fiber->set_status(Natalie::FiberObject::Status::Resumed);
    assert(fiber->block());
    Natalie::Value return_arg = nullptr;
    bool reraise = false;

    try {
        // NOTE: we cannot pass the env from this fiber (stack) across to another fiber (stack),
        // because doing so can cause unexpected results when env->caller() is used. (Since that
        // calling Env might have been overwritten in the other stack.)
        // That means a backtrace built from inside the fiber will end abruptly.
        // But that seems to be what Ruby does too.
        Natalie::Env e {};

        return_arg = NAT_RUN_BLOCK_WITHOUT_BREAK((&e), fiber->block(), Natalie::Args(fiber->args()), nullptr);
    } catch (Natalie::ExceptionObject *exception) {
        fiber->set_error(exception);
        reraise = true;
    }

    fiber->set_status(Natalie::FiberObject::Status::Terminated);
    fiber->set_end_of_stack(&fiber);

    if (reraise)
        fiber->swap_current(env, {});
    else
        fiber->swap_current(env, { return_arg });
}
}
