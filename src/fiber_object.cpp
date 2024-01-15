#include "natalie.hpp"
#include "natalie/thread_object.hpp"

#define MINICORO_IMPL
#include "minicoro.h"

namespace Natalie {

void *FiberObject::add_to_fiber_list(void *info) {
    auto item = (FiberObject *)info;
    if (s_list_size == 0) {
        s_list_size = 1;
        s_list = (FiberObject **)malloc(sizeof(FiberObject *));
    } else {
        s_list_size++;
        s_list = (FiberObject **)realloc(s_list, s_list_size * sizeof(FiberObject *));
    }
    s_list[s_list_size - 1] = item;
    return item;
}

void *FiberObject::remove_from_fiber_list(void *info) {
    assert(s_list_size != 0);
    auto item = (FiberObject *)info;
    bool found = false;
    for (size_t i = 0; i < s_list_size; ++i) {
        if (!found && s_list[i] == item) {
            found = true;
            continue;
        }
        if (found)
            s_list[i - 1] = s_list[i];
    }
    s_list_size--;
    if (s_list_size == 0) {
        free(s_list);
        s_list = nullptr;
    } else {
        s_list = (FiberObject **)realloc(s_list, s_list_size * sizeof(FiberObject *));
    }
    return item;
}

FiberObject *FiberObject::build_main_fiber(ThreadObject *thread, void *start_of_stack) {
    auto fiber = new FiberObject;
    assert(start_of_stack);
    fiber->m_start_of_stack = start_of_stack;
    fiber->m_thread = thread;
    return fiber;
}

bool FiberObject::is_current() const {
    return m_thread->current_fiber() == this;
}

bool FiberObject::is_main() const {
    return m_thread->main_fiber() == this;
}

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

    m_thread = ThreadObject::current();

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
    return current()->is_blocking() ? IntegerObject::create(1) : FalseObject::the();
}

Value FiberObject::ref(Env *env, Value key) {
    if (!key->is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key->klass()->inspect_str());
    auto fiber = current();
    while ((fiber->m_storage == nullptr || !fiber->m_storage->has_key(env, key)) && fiber->m_previous_fiber != nullptr)
        fiber = fiber->m_previous_fiber;
    if (fiber->m_storage == nullptr)
        return NilObject::the();
    return fiber->m_storage->ref(env, key);
}

Value FiberObject::refeq(Env *env, Value key, Value value) {
    if (!key->is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key->klass()->inspect_str());
    if (current()->m_storage == nullptr)
        current()->m_storage = new HashObject {};
    if (!value || value->is_nil()) {
        current()->m_storage->remove(env, key);
    } else {
        current()->m_storage->refeq(env, key, value);
    }
    return value;
}

Value FiberObject::resume(Env *env, Args args) {
    if (m_thread != ThreadObject::current())
        env->raise("FiberError", "fiber called across threads");
    if (m_status == Status::Terminated)
        env->raise("FiberError", "dead fiber called");
    if (m_previous_fiber)
        env->raise("FiberError", "attempt to resume the current fiber");

    set_args(args);

    GC_alloc_lock();

    auto suspending_fiber = m_previous_fiber = current();
    ThreadObject::current()->m_current_fiber = this;
    ThreadObject::current()->set_start_of_stack(m_start_of_stack);

#ifdef __SANITIZE_ADDRESS__
    auto fake_stack = __asan_get_current_fake_stack();
    void *real_stack = __asan_addr_is_in_fake_stack(fake_stack, &args, nullptr, nullptr);
    suspending_fiber->m_end_of_stack = real_stack ? real_stack : &args;
    suspending_fiber->m_asan_fake_stack = __asan_get_current_fake_stack();
#else
    suspending_fiber->m_end_of_stack = &args;
#endif

    GC_stack_base base = { m_start_of_stack };
    GC_set_stackbottom(nullptr, &base);

    auto res = mco_resume(m_coroutine);
    assert(res == MCO_SUCCESS);

    GC_alloc_unlock();

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
    auto scheduler = ThreadObject::current()->fiber_scheduler();
    if (!scheduler)
        return NilObject::the();

    return scheduler;
}

bool FiberObject::scheduler_is_relevant() {
    if (FiberObject::current()->is_blocking())
        return false;

    auto scheduler = FiberObject::scheduler();
    return !scheduler->is_nil();
}

Value FiberObject::set_scheduler(Env *env, Value scheduler) {
    if (scheduler->is_nil()) {
        ThreadObject::current()->set_fiber_scheduler(nullptr);
    } else {
        TM::Vector<TM::String> required_methods { "block", "unblock", "kernel_sleep", "io_wait" };
        for (const auto &required_method : required_methods) {
            if (!scheduler->respond_to(env, SymbolObject::intern(required_method)))
                env->raise("ArgumentError", "Scheduler must implement #{}", required_method);
        }
        ThreadObject::current()->set_fiber_scheduler(scheduler);
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

    current_fiber->m_previous_fiber->set_args(args);

    GC_alloc_lock();
    current_fiber->m_end_of_stack = &args;
    current_fiber->swap_to_previous(env);
    mco_yield(mco_running());
    GC_alloc_unlock();

    auto fiber_args = FiberObject::current()->args();
    if (fiber_args.size() == 0) {
        return NilObject::the();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return new ArrayObject { fiber_args.size(), fiber_args.data() };
    }
}

void FiberObject::swap_to_previous(Env *env) {
    assert(m_previous_fiber);
    auto new_current = m_previous_fiber;
    ThreadObject::current()->m_current_fiber = new_current;
    ThreadObject::current()->set_start_of_stack(new_current->start_of_stack());

    GC_stack_base base = { new_current->start_of_stack() };
    GC_set_stackbottom(nullptr, &base);

    m_previous_fiber = nullptr;
}

void FiberObject::set_args(Args args) {
    m_args.clear();
    for (size_t i = 0; i < args.size(); ++i) {
        m_args.push(args[i]);
    }
}

HashObject *FiberObject::ensure_thread_storage() {
    if (!m_thread_storage)
        m_thread_storage = new HashObject {};
    return m_thread_storage;
}

FiberObject *FiberObject::current() {
    return ThreadObject::current()->current_fiber();
}

FiberObject *FiberObject::main() {
    return ThreadObject::current()->main_fiber();
}

}

extern "C" {

void fiber_wrapper_func(mco_coro *co) {
    GC_alloc_unlock();
    auto user_data = (coroutine_user_data *)co->user_data;
    auto env = user_data->env;
    auto fiber = user_data->fiber;
    Natalie::ThreadObject::current()->set_start_of_stack(fiber->start_of_stack());
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

    auto previous_fiber = fiber->previous_fiber();
    if (reraise) {
        previous_fiber->set_args({});
    } else {
        Natalie::Value args[] = { return_arg };
        previous_fiber->set_args(Natalie::Args(1, args));
    }

    GC_alloc_lock();

    fiber->set_status(Natalie::FiberObject::Status::Terminated);
    fiber->set_end_of_stack(&fiber);

    if (reraise)
        fiber->swap_to_previous(env);
    else
        fiber->swap_to_previous(env);
}
}
