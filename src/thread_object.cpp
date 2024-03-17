#include <chrono>
#include <pthread.h>
#include <signal.h>

#include "natalie.hpp"
#include "natalie/thread_object.hpp"

static void set_stack_for_thread(Natalie::ThreadObject *thread_object) {
    auto thread_id = pthread_self();
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
    Natalie::ThreadObject::remove_from_list(thread);
    thread->unlock_mutexes();
}

static void *nat_create_thread(void *thread_object) {
    NAT_THREAD_DEBUG("Thread launched %p", thread_object);

    auto thread = (Natalie::ThreadObject *)thread_object;
    Natalie::tl_current_thread = thread;

    thread->set_native_thread_handle(pthread_self());
#ifdef __APPLE__
    thread->set_mach_thread_port(mach_thread_self());
#endif
    thread->set_suspend_status(Natalie::ThreadObject::SuspendStatus::Running);

#ifdef __SANITIZE_ADDRESS__
    thread->set_asan_fake_stack(__asan_get_current_fake_stack());
#endif

    pthread_cleanup_push(nat_thread_finish, thread);
    set_stack_for_thread(thread);
    thread->build_main_fiber();
    Natalie::Env e {};

    // The thread is now "Active", which means enough of the setup code has
    // run that we should be able to handle anything the user code throws at us.
    thread->set_status(Natalie::ThreadObject::Status::Active);
    NAT_THREAD_DEBUG("Thread status set to Active %p", thread);

    auto args = thread->args();
    auto block = thread->block();

    try {
        // This is the guts of the thread --
        // the user code that does what we came here to do.
        auto return_value = NAT_RUN_BLOCK_WITHOUT_BREAK((&e), block, args, nullptr);

        // If we got here and the thread has an exception,
        // this is our last chance to raise it. The catch directly below
        // will catch this exception and do the right thing.
        auto exception = thread->exception();
        if (exception)
            e.raise_exception(exception);

        // Store the value and exit the thread!
        // The cleanup handler does some additional housekeeping
        // as this thread is exiting.
        thread->set_value(return_value);
        pthread_exit(return_value.object());

    } catch (Natalie::ExceptionObject *exception) {
        // This code handles exceptions not rescued by user code.
        // This is where we report the error (if Thread#report_on_exception is true),
        // handle SystemExit et al, and where we abort the whole program
        // if Thread.abort_on_exception or Thread#abort_on_exception is true.

        if (exception->klass() == Natalie::ThreadObject::thread_kill_class()) {
            // This is the special ThreadKillError that user code
            // should not see. This means Thread#kill was called and this
            // exception has bubbled all the way up to this point.

            // But wait! If the thread had a pending (unrescued) exception
            // prior to the kill, we might need to print that out. Since
            // the thread is getting killed, such an exception cannot
            // abort the program. Saved by the bell!
            auto pending_exception = exception->cause();
            if (pending_exception) {
                // Calling Thread#value and Thread#join need to raise this pending exception,
                // so we need to store it on the thread for later use.
                thread->set_exception(pending_exception);

                // Now we can print it or handle SystemExit.
                Natalie::handle_top_level_exception(&e, pending_exception, false);
            } else {
                // ThreadKillError shouldn't be visible to user code,
                // which means it cannot be returned from Thread#value or Thread#join,
                // so we set this back to null.
                thread->set_exception(nullptr);
            }

            // OK, all good. Now we can exit the thread and let the
            // cleanup handler do its thing.
            pthread_exit(exception);

        } else {
            // Calling Thread#value and Thread#join need to raise this exception,
            // so we need to store it on the thread for later use.
            thread->set_exception(exception);

            // This is a regular Ruby Exception.
            // We need to potentially print it and/or handle SystemExit.
            Natalie::handle_top_level_exception(&e, exception, false);

            // The user might have said we should abort the whole program
            // if any thread fails, so this is where we do that.
            if (thread->abort_on_exception() || Natalie::ThreadObject::global_abort_on_exception())
                Natalie::ThreadObject::main()->raise(&e, { exception });

            // Finally, we can exit the thread and run the cleanup handler.
            pthread_exit(exception);
        }
    }

    pthread_cleanup_pop(0);

    NAT_UNREACHABLE();
}

namespace Natalie {

thread_local ThreadObject *tl_current_thread = nullptr;

static Value validate_key(Env *env, Value key) {
    if (key->is_string())
        key = key->as_string()->to_sym(env);
    if (!key->is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key->klass()->inspect_str());
    return key;
}

Value ThreadObject::pass(Env *env) {
    check_current_exception(env);

    sched_yield();

    return NilObject::the();
}

ThreadObject *ThreadObject::current() {
    return tl_current_thread;
}

Value ThreadObject::thread_kill(Env *env, Value thread) {
    if (!thread->is_thread())
        env->raise("TypeError", "wrong argument type {} (expected VM/thread)", thread->klass()->inspect_str());

    return thread->as_thread()->kill(env);
}

Value ThreadObject::stop(Env *env) {
    tl_current_thread->sleep(env, -1.0);
    return NilObject::the();
}

bool ThreadObject::is_stopped() const {
    return m_sleeping || m_status == Status::Dead;
}

void ThreadObject::build_main_thread(Env *env, void *start_of_stack) {
    assert(!s_main); // can only be built once
    auto thread = new ThreadObject;
    assert(start_of_stack);
    thread->m_start_of_stack = start_of_stack;
    thread->m_status = ThreadObject::Status::Active;
    thread->m_suspend_status = ThreadObject::SuspendStatus::Running;
    ThreadGroupObject::get_default()->add(env, thread);
    set_stack_for_thread(thread);
    thread->build_main_fiber();
    s_main = thread;
    tl_current_thread = thread;
    add_to_list(thread);
    setup_interrupt_pipe(env);
}

void ThreadObject::build_main_fiber() {
    m_current_fiber = m_main_fiber = FiberObject::build_main_fiber(this, m_start_of_stack);
}

ThreadObject *ThreadObject::initialize(Env *env, Args args, Block *block) {
    if (this == ThreadObject::main())
        env->raise("ThreadError", "already initialized");

    if (m_block)
        env->raise("ThreadError", "already initialized");

    if (!block)
        env->raise("ThreadError", "must be called with a block");

    add_to_list(this);
    ThreadGroupObject::get_default()->add(env, this);

    // DANGER ZONE ===========================================
    // Do not allocate any GC-managed object after this point
    // in this function. The ThreadObject::add_to_list() call
    // is protected by mutex, thus any thread added to the
    // list is guaranteed to be seen by the Garbage Collector
    // and the GC will wait until:
    //
    // 1. The thread is running (m_suspend_status == SuspendStatus::Running)
    // 2. The thread is subsequently stopped by a SIGUSR1 signal.
    //
    // If you allocate GC-managed memory beyond this point,
    // you *will* lock the GC, and this thread (and thus the
    // whole program) will hang forever.

    m_context = (ucontext_t *)malloc(sizeof(ucontext_t));

    m_args = args;
    m_block = block;

    m_file = env->file();
    m_line = env->line();

    m_report_on_exception = s_report_on_exception;

    NAT_THREAD_DEBUG("Creating thread %p", this);
    m_thread = std::thread { nat_create_thread, (void *)this };
    m_native_thread_handle = m_thread.native_handle();

    // END DANGER ZONE =======================================

    return this;
}

Value ThreadObject::to_s(Env *env) {
    String location;

    if (m_file && m_line)
        location = String::format(" {}:{}", *m_file, *m_line);

    auto formatted = String::format(
        "#<{}:{}{} {}>",
        m_klass->inspect_str(),
        String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed),
        location,
        status());

    return new StringObject { formatted, Encoding::ASCII_8BIT };
}

Value ThreadObject::status(Env *env) {
    auto status_string = status();
    if (m_status == Status::Dead) {
        if (m_exception)
            return NilObject::the();
        return FalseObject::the();
    }
    return new StringObject { status_string };
}

String ThreadObject::status() {
    switch (m_status) {
    case Status::Created:
        return "run";
    case Status::Active:
        return m_sleeping ? "sleep" : "run";
    case Status::Aborting:
        return m_sleeping ? "sleep" : "aborting";
    case Status::Dead:
        return "dead";
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
            fprintf(stderr, "Unable to join thread: %s (%d)", e.what(), e.code().value());
            abort();
        }
    }

    m_joined = true;

    if (m_exception)
        env->raise_exception(m_exception);

    return this;
}

Value ThreadObject::kill(Env *env) {
    if (is_main()) {
        // FIXME: raise SystemExit
        exit(0);
    }

    if (m_status == Status::Dead)
        return this;

    wait_until_running();

    m_status = Status::Aborting;

    auto exception = new ExceptionObject { thread_kill_class(env) };

    if (m_exception) {
        // An pending exception was already raised on this thread,
        // and we might need to print it out. We'll store it in the "cause"
        // slot for now, but this might need a dedicated holder in the future.
        exception->set_cause(m_exception);
    }

    if (is_current()) {
        env->raise_exception(exception);
    } else {
        m_exception = exception;
        wakeup(env);
        ThreadObject::interrupt();
    }

    return this;
}

Value ThreadObject::raise(Env *env, Args args) {
    if (m_status == Status::Dead)
        return NilObject::the();

    auto exception = ExceptionObject::create_for_raise(env, std::move(args), nullptr, false);

    if (is_current())
        env->raise_exception(exception);

    m_exception = exception;

    // Wake up the thread in case it is sleeping.
    {
        std::unique_lock sleep_lock { m_sleep_lock };
        m_sleep_cond.notify_one();
    }

    // In case this thread is blocking on read/select/whatever,
    // we may need to interrupt it (and all other threads, incidentally).
    ThreadObject::interrupt();

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

    {
        std::unique_lock sleep_lock { m_sleep_lock };
        m_sleep_cond.notify_one();
    }

    return this;
}

Value ThreadObject::sleep(Env *env, float timeout) {
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

    if (timeout < 0.0) {
        {
            std::unique_lock sleep_lock { m_sleep_lock };

            check_exception(env);

            Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
            ThreadObject::set_current_sleeping(true);

            m_sleep_cond.wait(sleep_lock);
        }

        check_exception(env);

        return calculate_elapsed();
    }

    const auto wait = std::chrono::nanoseconds(static_cast<int64_t>(timeout * 1000 * 1000 * 1000));
    {
        std::unique_lock sleep_lock { m_sleep_lock };

        check_exception(env);

        Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
        ThreadObject::set_current_sleeping(true);

        m_sleep_cond.wait_for(sleep_lock, wait);
    }

    check_exception(env);

    return calculate_elapsed();
}

Value ThreadObject::value(Env *env) {
    join(env);

    if (!m_value)
        return NilObject::the();

    return m_value;
}

Value ThreadObject::name(Env *env) {
    if (!m_name)
        return NilObject::the();
    return new StringObject { *m_name };
}

Value ThreadObject::set_name(Env *env, Value name) {
    if (!name || name->is_nil()) {
        m_name.clear();
        return NilObject::the();
    }

    auto name_str = name->to_str(env);
    if (strlen(name_str->c_str()) != name_str->bytesize())
        env->raise("ArgumentError", "string contains null byte");
    m_name = name_str->string();
    return name;
}

Value ThreadObject::priority(Env *env) const {
    return Value::integer(m_priority);
}

// Example code: https://en.cppreference.com/w/cpp/thread/thread/native_handle
Value ThreadObject::set_priority(Env *env, Value priority) {
    auto priority_int = priority->to_int(env);
    if (priority_int->is_bignum())
        env->raise("RangeError", "bignum too big to convert into `long'");

    m_priority = priority_int->to_nat_int_t();
    if (m_priority > 3) m_priority = 3;
    if (m_priority < -3) m_priority = -3;

    sched_param sch;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &sch);
    sch.sched_priority = m_priority;
    // Ignore errors
    pthread_setschedparam(pthread_self(), SCHED_RR, &sch);

    return priority;
}

Value ThreadObject::fetch(Env *env, Value key, Value default_value, Block *block) {
    key = validate_key(env, key);
    HashObject *hash = nullptr;
    if (m_current_fiber)
        hash = m_current_fiber->thread_storage();
    if (!hash)
        hash = new HashObject {};
    return hash->fetch(env, key, default_value, block);
}

bool ThreadObject::has_key(Env *env, Value key) {
    key = validate_key(env, key);
    HashObject *hash = nullptr;
    if (m_current_fiber)
        hash = m_current_fiber->thread_storage();
    if (!hash)
        return false;
    return hash->has_key(env, key);
}

Value ThreadObject::keys(Env *env) {
    HashObject *hash = nullptr;
    if (m_current_fiber)
        hash = m_current_fiber->thread_storage();
    if (!hash)
        return new ArrayObject {};
    return hash->keys(env);
}

Value ThreadObject::ref(Env *env, Value key) {
    key = validate_key(env, key);
    HashObject *hash = nullptr;
    if (m_current_fiber)
        hash = m_current_fiber->thread_storage();
    if (!hash)
        return NilObject::the();
    return hash->ref(env, key);
}

Value ThreadObject::refeq(Env *env, Value key, Value value) {
    if (is_frozen())
        env->raise("FrozenError", "can't modify frozen thread locals");
    key = validate_key(env, key);
    assert(m_current_fiber);
    auto hash = m_current_fiber->ensure_thread_storage();
    hash->refeq(env, key, value);
    return value;
}

bool ThreadObject::has_thread_variable(Env *env, Value key) const {
    if (!key->is_symbol() && !key->is_string() && key->respond_to(env, "to_str"_s))
        key = key->to_str(env);
    if (key->is_string())
        key = key->as_string()->to_sym(env);
    return m_thread_variables && m_thread_variables->has_key(env, key);
}

Value ThreadObject::thread_variable_get(Env *env, Value key) {
    if (!m_thread_variables)
        return NilObject::the();
    if (!key->is_symbol() && !key->is_string() && key->respond_to(env, "to_str"_s))
        key = key->to_str(env);
    if (key->is_string())
        key = key->as_string()->to_sym(env);
    return m_thread_variables->ref(env, key);
}

Value ThreadObject::thread_variable_set(Env *env, Value key, Value value) {
    if (is_frozen())
        env->raise("FrozenError", "can't modify frozen thread locals");
    if (!key->is_symbol() && !key->is_string() && key->respond_to(env, "to_str"_s))
        key = key->to_str(env);
    if (key->is_string())
        key = key->as_string()->to_sym(env);
    if (!key->is_symbol())
        env->raise("TypeError", "{} is not a symbol", key->inspect_str(env));
    if (!m_thread_variables)
        m_thread_variables = new HashObject;
    return m_thread_variables->refeq(env, key, value);
}

Value ThreadObject::thread_variables(Env *env) const {
    if (!m_thread_variables)
        return new ArrayObject;
    return m_thread_variables->keys(env);
}

Value ThreadObject::list(Env *env) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    auto ary = new ArrayObject { s_list.size() };
    for (auto thread : s_list) {
        if (thread->m_status != ThreadObject::Status::Dead)
            ary->push(thread);
    }
    return ary;
}

void ThreadObject::add_to_list(ThreadObject *thread) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    s_list.push(thread);
}

void ThreadObject::remove_from_list(ThreadObject *thread) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    size_t i;
    bool found = false;
    for (i = 0; i < s_list.size(); ++i) {
        if (s_list.at(i) == thread) {
            found = true;
            break;
        }
    }
    assert(found);
    s_list.remove(i);
}

void ThreadObject::add_mutex(Thread::MutexObject *mutex) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    m_mutexes.set(mutex);
}

void ThreadObject::remove_mutex(Thread::MutexObject *mutex) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    m_mutexes.remove(mutex);
}

void ThreadObject::unlock_mutexes() const {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    for (auto pair : m_mutexes)
        pair.first->unlock_without_checks();
}

void ThreadObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    for (auto arg : m_args.vector())
        visitor.visit(arg);
    visitor.visit(m_block);
    visitor.visit(m_exception);
    visitor.visit(m_value);
    visitor.visit(m_thread_variables);
    for (auto pair : m_mutexes)
        visitor.visit(pair.first);
    visitor.visit(m_fiber_scheduler);
    visitor.visit(s_thread_kill_class);
    visitor.visit(m_group);

    visitor.visit(m_current_fiber);
    visitor.visit(m_main_fiber);

    // If this thread is Dead, then it's possible the OS has already reclaimed
    // the stack space. We shouldn't have any remaining variables on the stack
    // that we need to keep anyway.
    if (m_status == Status::Dead)
        return;

    visit_children_from_stack(visitor);
}

// If the thread status is Status::Created, it means its execution
// has not reached the try/catch handler yet. We shouldn't do
// anything to the thread that might cause it to terminate
// until its status is Status::Active.
void ThreadObject::wait_until_running() const {
    while (m_status == Status::Created)
        sched_yield();
}

void ThreadObject::setup_interrupt_pipe(Env *env) {
    int pipefd[2];
    if (pipe2(pipefd, O_CLOEXEC | O_NONBLOCK) < 0)
        env->raise_errno();
    s_interrupt_read_fileno = pipefd[0];
    s_interrupt_write_fileno = pipefd[1];
}

void ThreadObject::interrupt() {
    ::write(s_interrupt_write_fileno, "!", 1);
}

void ThreadObject::clear_interrupt() {
    // arbitrarily-chosen buffer size just to avoid several single-byte reads
    constexpr int BUF_SIZE = 8;
    char buf[BUF_SIZE];
    ssize_t bytes;
    do {
        // This fd is non-blocking, so this can set errno to EAGAIN,
        // but we don't care -- we just want the buffer to be empty.
        bytes = ::read(s_interrupt_read_fileno, buf, BUF_SIZE);
    } while (bytes > 0);
}

void ThreadObject::check_current_exception(Env *env) {
    current()->check_exception(env);
}

void ThreadObject::check_exception(Env *env) {
    if (!m_exception)
        return;

    auto exception = m_exception.load();

    if (exception->klass() == s_thread_kill_class) {
        m_exception = nullptr;
        env->raise_exception(exception);
    }

    exception = Value(exception).send(env, "exception"_s, {})->as_exception_or_raise(env);
    m_exception = nullptr;
    env->raise_exception(exception);
}

void ThreadObject::detach_all() {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    for (auto thread : s_list) {
        if (thread->is_main())
            continue;
        thread->detach();
    }
}

void ThreadObject::stop_the_world_and_save_context() {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    assert(current()->is_main());

    NAT_THREAD_DEBUG("waiting till all threads have launched");
    bool all_launched;
    do {
        all_launched = true;
        for (auto thread : ThreadObject::list()) {
            if (thread->suspend_status() == SuspendStatus::Launching) {
                all_launched = false;
                break;
            }
        }
        sched_yield();
    } while (!all_launched);

    NAT_THREAD_DEBUG("sending signals to suspend all threads except main");
    for (auto thread : ThreadObject::list()) {
        if (thread->is_main()) continue;
        thread->suspend();
    }

    NAT_THREAD_DEBUG("waiting for threads to suspend");
    bool ready = false;
    while (!ready) {
        ready = true;
        for (auto thread : ThreadObject::list()) {
            if (thread->is_main()) continue;
            if (thread->suspend_status() == SuspendStatus::Suspended) {
                continue;
            } else {
                ready = false;
                break;
            }
        }
        NAT_THREAD_DEBUG("spinning...");
        sched_yield();
    }

    NAT_THREAD_DEBUG("all threads indicated they have suspended");
}

void ThreadObject::wake_up_the_world() {
    NAT_THREAD_DEBUG("sending signals to resume all threads except main");
    for (auto thread : ThreadObject::list()) {
        if (thread->is_main()) continue;
        thread->resume();
    }

    NAT_THREAD_DEBUG("waiting for threads to resume");
    bool ready = false;
    while (!ready) {
        ready = true;
        for (auto thread : ThreadObject::list()) {
            if (thread->is_main()) continue;
            if (thread->suspend_status() == SuspendStatus::Running) {
                continue;
            } else {
                ready = false;
                break;
            }
        }
        NAT_THREAD_DEBUG("spinning...");
        sched_yield();
    }

    NAT_THREAD_DEBUG("all threads indicated they have resumed");
}

void ThreadObject::suspend() { // NOLINT "can be made const" warning only for Linux
#ifdef __APPLE__
    assert(m_mach_thread_port != MACH_PORT_NULL);
    int result;
    do {
        result = thread_suspend(m_mach_thread_port);
    } while (result == KERN_ABORTED);
    assert(result == KERN_SUCCESS);
    m_suspend_status = SuspendStatus::Suspended;
#else
    assert(m_native_thread_handle);
    pthread_kill(m_native_thread_handle, SIGUSR1);
    NAT_THREAD_DEBUG("sent suspend signal to thread %p", this);
#endif
}

void ThreadObject::resume() { // NOLINT "can be made const" warning only for Linux
#ifdef __APPLE__
    assert(m_mach_thread_port != MACH_PORT_NULL);
    int result;
    do {
        result = thread_resume(m_mach_thread_port);
    } while (result == KERN_ABORTED);
    assert(result == KERN_SUCCESS);
    m_suspend_status = SuspendStatus::Running;
#else
    assert(m_native_thread_handle);
    pthread_kill(m_native_thread_handle, SIGUSR2);
    NAT_THREAD_DEBUG("sent resume signal to thread %p", this);
#endif
}

NO_SANITIZE_ADDRESS void ThreadObject::visit_children_from_stack(Visitor &visitor) const {
    // If this is the currently active thread,
    // we don't need walk its stack a second time.
    if (tl_current_thread == this)
        return;

    // If this thread is still in the state of being setup, the stack might not be
    // known yet. Plus, there shouldn't be any GC-managed variables on the stack yet.
    if (m_suspend_status == SuspendStatus::Launching)
        return;

    // Walk the stack looking for variables...
    for (char *ptr = reinterpret_cast<char *>(m_end_of_stack); ptr < m_start_of_stack; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
        if (Heap::the().is_a_heap_cell_in_use(potential_cell))
            visitor.visit(potential_cell);
#ifdef __SANITIZE_ADDRESS__
        visit_children_from_asan_fake_stack(visitor, potential_cell);
#endif
    }

    // Check in the registers for any variables...
    visit_children_from_context(visitor);
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

NO_SANITIZE_ADDRESS void ThreadObject::visit_children_from_context(Visitor &visitor) const {
    for (char *i = (char *)m_context; i < (char *)m_context + sizeof(ucontext_t); ++i) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(i);
        if (!potential_cell)
            continue;
        if (Heap::the().is_a_heap_cell_in_use(potential_cell))
            visitor.visit(potential_cell);
    }
}

}
