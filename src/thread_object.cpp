#include <signal.h>
#include <sys/time.h>

#include "natalie.hpp"
#include "natalie/thread_object.hpp"

thread_local Natalie::ThreadObject *tl_current_thread = nullptr;

static void set_stack_for_thread(pthread_t thread_id, Natalie::ThreadObject *thread_object) {
#if defined(__APPLE__)
    auto start = pthread_get_stackaddr_np(thread_id);
    assert(start);
    auto stack_size = pthread_get_stacksize_np(thread_id);
    thread_object->set_start_of_stack(start);
    thread_object->set_end_of_stack((void *)((uintptr_t)start - stack_size + 8));
#else
    void *end;
    size_t stack_size;
    pthread_attr_t attr;
    pthread_getattr_np(thread_id, &attr);
    pthread_attr_getstack(&attr, &end, &stack_size);
    thread_object->set_start_of_stack((void *)((uintptr_t)end + stack_size));
    thread_object->set_end_of_stack(end);
    pthread_attr_destroy(&attr);
#endif
}

static void nat_thread_finish(void *thread_object) {
    auto thread = (Natalie::ThreadObject *)thread_object;
    thread->set_status(Natalie::ThreadObject::Status::Dead);
    thread->remove_from_list();
    thread->unlock_mutexes();
}

static void *nat_create_thread(void *thread_object) {
    auto thread = (Natalie::ThreadObject *)thread_object;
#ifdef __SANITIZE_ADDRESS__
    thread->set_asan_fake_stack(__asan_get_current_fake_stack());
#endif

    pthread_cleanup_push(nat_thread_finish, thread);

    auto thread_id = pthread_self();

    // NOTE: We set the thread_id again here because *sometimes* the new thread
    // starts executing *before* pthread_create() sets the thread_id. Fun!
    thread->set_thread_id(thread_id);

    set_stack_for_thread(thread_id, thread);
    tl_current_thread = thread;

    thread->build_main_fiber();
    thread->set_status(Natalie::ThreadObject::Status::Active);

    auto block = thread->block();

    Natalie::Env e {};
    try {
        auto return_value = NAT_RUN_BLOCK_WITHOUT_BREAK((&e), block, Natalie::Args(), nullptr);
        thread->set_value(return_value);
        pthread_exit(nullptr);
    } catch (Natalie::ExceptionObject *exception) {
        Natalie::handle_top_level_exception(&e, exception, false);
        thread->set_exception(exception);
        pthread_exit(Natalie::NilObject::the());
    }

    pthread_cleanup_pop(0);

    NAT_UNREACHABLE();
}

namespace Natalie {

std::mutex g_thread_mutex;
std::recursive_mutex g_thread_recursive_mutex;

Value ThreadObject::pass() {
    // We need a cancellation point here so the thread can be interrupted.
    usleep(0);

    sched_yield();

    return NilObject::the();
}

ThreadObject *ThreadObject::current() {
    return tl_current_thread;
}

Value ThreadObject::stop(Env *env) {
    tl_current_thread->sleep(env, -1.0);
    return NilObject::the();
}

bool ThreadObject::is_stopped() const {
    return m_sleeping || m_status == Status::Dead;
}

void ThreadObject::build_main_thread(void *start_of_stack) {
    assert(!s_main && !s_main_id); // can only be built once
    auto thread = new ThreadObject;
    assert(start_of_stack);
    thread->m_start_of_stack = start_of_stack;
    thread->m_status = ThreadObject::Status::Active;
    thread->m_thread_id = pthread_self();
    set_stack_for_thread(thread->m_thread_id, thread);
    thread->build_main_fiber();
    s_main = thread;
    s_main_id = thread->m_thread_id;
    tl_current_thread = thread;
}

void ThreadObject::build_main_fiber() {
    m_current_fiber = m_main_fiber = FiberObject::build_main_fiber(this, m_start_of_stack);
}

ThreadObject *ThreadObject::initialize(Env *env, Block *block) {
    assert(this != ThreadObject::main()); // can never be main thread

    env->ensure_block_given(block);
    m_block = block;

    m_file = env->file();
    m_line = env->line();

    m_thread = std::thread { nat_create_thread, (void *)this };

    return this;
}

Value ThreadObject::status(Env *env) {
    switch (m_status) {
    case Status::Created:
        return new StringObject { "run" };
    case Status::Active:
        return new StringObject { m_sleeping ? "sleep" : "run" };
    case Status::Dead:
        if (m_exception)
            return NilObject::the();
        return FalseObject::the();
    }
    NAT_UNREACHABLE();
}

Value ThreadObject::join(Env *env) {
    if (is_current())
        env->raise("ThreadError", "Target thread must not be current thread");

    if (is_main())
        env->raise("ThreadError", "Target thread must not be main thread");

    wait_until_running();

    if (m_joined)
        return this;

    try {
        m_thread.join();
    } catch (std::system_error &e) {
        if (e.code() == std::errc::invalid_argument) {
            // no biggie - thread was already joined
        } else {
            printf("Unable to join thread: %s (%d)", e.what(), e.code().value());
            abort();
        }
    }

    m_joined = true;
    remove_from_list();

    return this;
}

Value ThreadObject::kill(Env *) const {
    if (is_main())
        exit(0);

    wait_until_running();

    std::lock_guard<std::recursive_mutex> lock(g_thread_recursive_mutex);
    pthread_kill(m_thread_id, SIGINT);
    remove_from_list();

    return NilObject::the();
}

Value ThreadObject::raise(Env *env, Value klass, Value message) {
    if (klass && klass->is_string()) {
        message = klass;
        klass = nullptr;
    }
    if (!klass)
        klass = GlobalEnv::the()->Object()->const_fetch("RuntimeError"_s);
    auto exception = new ExceptionObject { klass->as_class_or_raise(env), new StringObject { "" } };
    if (is_main()) {
        env->raise_exception(exception);
    } else {
        m_exception = exception;
        kill(env);
    }
    return NilObject::the();
}

Value ThreadObject::run(Env *env) {
    wakeup(env);
    return NilObject::the();
}

Value ThreadObject::wakeup(Env *env) {
    if (m_status == Status::Dead)
        env->raise("ThreadError", "killed thread");

    wait_until_running();

    pthread_mutex_lock(&m_sleep_lock);
    pthread_cond_signal(&m_sleep_cond);
    pthread_mutex_unlock(&m_sleep_lock);

    return NilObject::the();
}

Value ThreadObject::sleep(Env *env, float timeout) {
    Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
    ThreadObject::set_current_sleeping(true);

    timespec t_begin;
    if (::clock_gettime(CLOCK_MONOTONIC, &t_begin) < 0)
        env->raise_errno();

    auto calculate_elapsed = [&t_begin, &env] {
        timespec t_end;
        if (::clock_gettime(CLOCK_MONOTONIC, &t_end) < 0)
            env->raise_errno();
        int elapsed = t_end.tv_sec - t_begin.tv_sec;
        if (t_end.tv_nsec < t_begin.tv_nsec) elapsed--;
        return Value::integer(elapsed);
    };

    auto handle_error = [&env](int result) {
        switch (result) {
        case 0:
        case ETIMEDOUT:
            return;
        case EINVAL:
            env->raise("ThreadError", "EINVAL");
        case EPERM:
            env->raise("ThreadError", "EPERM");
        default:
            env->raise("ThreadError", "unknown error");
        }
    };

    if (ThreadObject::i_am_main()) {
        if (timeout < 0.0) {
            while (true)
                ::sleep(10000);
        } else {
            struct timespec ts;
            ts.tv_sec = ::floor(timeout);
            ts.tv_nsec = (timeout - ts.tv_sec) * 1000000000;
            ::nanosleep(&ts, nullptr);
        }
        return calculate_elapsed();
    }

    if (timeout < 0.0) {
        pthread_mutex_lock(&m_sleep_lock);
        handle_error(pthread_cond_wait(&m_sleep_cond, &m_sleep_lock));
        pthread_mutex_unlock(&m_sleep_lock);
        return calculate_elapsed();
    }

    timespec wait;
    timeval now;
    if (gettimeofday(&now, nullptr) != 0)
        env->raise_errno();
    wait.tv_sec = now.tv_sec + (int)timeout;
    auto us = (timeout - (int)timeout) * 1000 * 1000;
    auto ns = (now.tv_usec + us) * 1000;
    constexpr long ns_in_sec = 1000 * 1000 * 1000;
    if (ns >= ns_in_sec) {
        wait.tv_sec += 1;
        ns -= ns_in_sec;
    }
    wait.tv_nsec = ns;

    pthread_mutex_lock(&m_sleep_lock);
    handle_error(pthread_cond_timedwait(&m_sleep_cond, &m_sleep_lock, &wait));
    pthread_mutex_unlock(&m_sleep_lock);

    return calculate_elapsed();
}

Value ThreadObject::value(Env *env) {
    join(env);

    if (!m_value)
        return NilObject::the();

    return m_value;
}

Value ThreadObject::ref(Env *env, Value key) {
    if (!key->is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key->klass()->inspect_str());
    if (!m_storage)
        return NilObject::the();
    return m_storage->ref(env, key);
}

Value ThreadObject::refeq(Env *env, Value key, Value value) {
    if (!key->is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key->klass()->inspect_str());
    if (!m_storage)
        m_storage = new HashObject {};
    m_storage->refeq(env, key, value);
    return value;
}

Value ThreadObject::list(Env *env) {
    std::lock_guard<std::mutex> lock(g_thread_mutex);
    auto ary = new ArrayObject { s_list.size() };
    for (auto thread : s_list) {
        if (thread->m_status != ThreadObject::Status::Dead)
            ary->push(thread);
    }
    return ary;
}

void ThreadObject::remove_from_list() const {
    std::lock_guard<std::recursive_mutex> lock(g_thread_recursive_mutex);
    size_t i;
    bool found = false;
    for (i = 0; i < s_list.size(); ++i) {
        if (s_list.at(i) == this) {
            found = true;
            break;
        }
    }
    // We call remove_from_list() from a few different places,
    // so it's possible it was already removed.
    if (found)
        s_list.remove(i);
}

void ThreadObject::add_mutex(Thread::MutexObject *mutex) {
    std::lock_guard<std::mutex> lock(g_thread_mutex);

    m_mutexes.set(mutex);
}

void ThreadObject::remove_mutex(Thread::MutexObject *mutex) {
    std::lock_guard<std::mutex> lock(g_thread_mutex);

    m_mutexes.remove(mutex);
}

void ThreadObject::unlock_mutexes() const {
    std::lock_guard<std::mutex> lock(g_thread_mutex);

    for (auto pair : m_mutexes)
        pair.first->unlock_without_checks();
}

void ThreadObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_block);
    visitor.visit(m_current_fiber);
    visitor.visit(m_exception);
    visitor.visit(m_main_fiber);
    visitor.visit(m_storage);
    visitor.visit(m_value);
    for (auto pair : m_mutexes)
        visitor.visit(pair.first);
    visitor.visit(m_fiber_scheduler);
    visit_children_from_stack(visitor);
}

// If the thread status is Status::Created, it means we maybe haven't
// set up our signal handler for it yet, so we should give
// it a chance to switch to Status::Active.
void ThreadObject::wait_until_running() const {
    while (m_status == Status::Created)
        sched_yield();
}

NO_SANITIZE_ADDRESS void ThreadObject::visit_children_from_stack(Visitor &visitor) const {
    if (pthread_self() == m_thread_id)
        return; // this is the currently active thread, so don't walk its stack a second time
    if (m_status != Status::Active)
        return;

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
NO_SANITIZE_ADDRESS void ThreadObject::visit_children_from_asan_fake_stack(Visitor &visitor, Cell *potential_cell) const {
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
void ThreadObject::visit_children_from_asan_fake_stack(Visitor &visitor, Cell *potential_cell) const { }
#endif
}
