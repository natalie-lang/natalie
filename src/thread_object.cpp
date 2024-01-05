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

    auto args = thread->args();
    auto block = thread->block();

    Natalie::Env e {};

    try {
        thread->set_status(Natalie::ThreadObject::Status::Active);
        auto return_value = NAT_RUN_BLOCK_WITHOUT_BREAK((&e), block, args, nullptr);
        thread->cancelation_checkpoint(&e);
        thread->set_value(return_value);
        pthread_exit(return_value.object());
    } catch (Natalie::ExceptionObject *exception) {
        Natalie::handle_top_level_exception(&e, exception, false);
        thread->set_exception(exception);
        if (thread->abort_on_exception() || Natalie::ThreadObject::global_abort_on_exception())
            Natalie::ThreadObject::main()->raise(&e, { exception });
        pthread_exit(exception);
    }

    pthread_cleanup_pop(0);

    NAT_UNREACHABLE();
}

namespace Natalie {

static Value validate_key(Env *env, Value key) {
    if (key->is_string())
        key = key->as_string()->to_sym(env);
    if (!key->is_symbol())
        env->raise("TypeError", "wrong argument type {} (expected Symbol)", key->klass()->inspect_str());
    return key;
}

std::mutex g_thread_mutex;
std::recursive_mutex g_thread_recursive_mutex;

Value ThreadObject::pass(Env *env) {
    cancelation_checkpoint(env);

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

void ThreadObject::build_main_thread(Env *env, void *start_of_stack) {
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
    thread->setup_signal_pipe(env);
}

void ThreadObject::build_main_fiber() {
    m_current_fiber = m_main_fiber = FiberObject::build_main_fiber(this, m_start_of_stack);
}

ThreadObject *ThreadObject::initialize(Env *env, Args args, Block *block) {
    if (this == ThreadObject::main())
        env->raise("ThreadError", "already initialized");

    if (m_args)
        env->raise("ThreadError", "already initialized");

    if (!block)
        env->raise("ThreadError", "must be called with a block");
    m_args = args.to_array();
    m_block = block;

    m_file = env->file();
    m_line = env->line();

    setup_signal_pipe(env);

    m_thread = std::thread { nat_create_thread, (void *)this };

    m_report_on_exception = s_report_on_exception;

    return this;
}

void ThreadObject::setup_signal_pipe(Env *env) {
    int pipefd[2];
    if (pipe2(pipefd, O_CLOEXEC | O_NONBLOCK) < 0)
        env->raise_errno();
    m_signal_read_fileno = pipefd[0];
    m_signal_write_fileno = pipefd[1];
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

    return new StringObject { formatted, EncodingObject::get(Encoding::ASCII_8BIT) };
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
        return "aborting";
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
    remove_from_list();

    if (m_exception)
        env->raise_exception(m_exception);

    return this;
}

Value ThreadObject::kill(Env *env) {
    if (is_main())
        exit(0);

    wait_until_running();

    m_status = Status::Aborting;

    std::lock_guard<std::recursive_mutex> lock(g_thread_recursive_mutex);
    pthread_cancel(m_thread_id);
    remove_from_list();

    return NilObject::the();
}

Value ThreadObject::raise(Env *env, Args args) {
    if (m_status == Status::Dead)
        return NilObject::the();

    auto exception = ExceptionObject::create_for_raise(env, std::move(args), nullptr, false);

    if (is_current())
        env->raise_exception(exception);

    m_exception = exception;
    wakeup(env);

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

    write_signal();

    return NilObject::the();
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

    if (timeout < 0.0) {
        pthread_mutex_lock(&m_sleep_lock);

        Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
        ThreadObject::set_current_sleeping(true);

        handle_error(pthread_cond_wait(&m_sleep_cond, &m_sleep_lock));
        pthread_mutex_unlock(&m_sleep_lock);

        cancelation_checkpoint(env);

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

    Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
    ThreadObject::set_current_sleeping(true);

    handle_error(pthread_cond_timedwait(&m_sleep_cond, &m_sleep_lock, &wait));
    pthread_mutex_unlock(&m_sleep_lock);

    cancelation_checkpoint(env);

    return calculate_elapsed();
}

Value ThreadObject::value(Env *env) {
    join(env);

    if (m_exception)
        env->raise_exception(m_exception);

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

Value ThreadObject::fetch(Env *env, Value key, Value default_value, Block *block) {
    key = validate_key(env, key);
    HashObject *hash = m_storage;
    if (!hash)
        hash = new HashObject {};
    return hash->fetch(env, key, default_value, block);
}

bool ThreadObject::has_key(Env *env, Value key) {
    key = validate_key(env, key);
    if (!m_storage)
        return false;
    return m_storage->has_key(env, key);
}

Value ThreadObject::keys(Env *env) {
    if (!m_storage)
        return new ArrayObject {};
    return m_storage->keys(env);
}

Value ThreadObject::ref(Env *env, Value key) {
    key = validate_key(env, key);
    if (!m_storage)
        return NilObject::the();
    return m_storage->ref(env, key);
}

Value ThreadObject::refeq(Env *env, Value key, Value value) {
    if (is_frozen())
        env->raise("FrozenError", "can't modify frozen thread locals");
    key = validate_key(env, key);
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
    visitor.visit(m_args);
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

// If the thread status is Status::Created, it means its execution
// has not reached the try/catch handler yet. We shouldn't do
// anything to the thread that might cause it to terminate
// until its status is Status::Active.
void ThreadObject::wait_until_running() const {
    while (m_status == Status::Created)
        sched_yield();
}

void ThreadObject::write_signal() const {
    ::write(m_signal_write_fileno, "!", 1);
}

// It would be wrong if some code called Thread#wakeup a dozen times
// and then the next dozen times the thread slept, it woke up immediately,
// so let's clear the buffer after each time the thread gets signaled.
void ThreadObject::clear_signal() const {
    // arbitrarily-chosen buffer size just to avoid several single-byte reads
    constexpr int BUF_SIZE = 8;
    char buf[BUF_SIZE];
    ssize_t bytes;
    do {
        // This fd is non-blocking, so this can set errno to EAGAIN,
        // but we don't care -- we just want the buffer to be empty.
        bytes = ::read(m_signal_read_fileno, buf, BUF_SIZE);
    } while (bytes > 0);
}

void ThreadObject::wait_for_ready_fd(Env *env, int fd) {
    auto thread = current();
    auto signal_fd = IoObject::read_signal_fd(fd);
    auto thread_signal_fd = thread->m_signal_read_fileno;

    fd_set readfds;
    FD_ZERO(&readfds);

    // This is the file/socket/whatever we care about.
    // We would like to start reading from this fd when it's ready!
    FD_SET(fd, &readfds);
    auto nfds = fd + 1;

    // This is the sidecar signal fd that goes with the fd above.
    // This lets us wake up before the fd gets closed.
    // (Because Linux doesn't return from select(), poll(), or epoll(),
    // when the fd gets closed. That really sucks.)
    FD_SET(signal_fd, &readfds);
    nfds = std::max(nfds, signal_fd + 1);

    // This is the signal fd for the current thread.
    // If Thread#wakeup gets called on this thread, this signals us to wake up.
    FD_SET(thread_signal_fd, &readfds);
    nfds = std::max(nfds, thread_signal_fd + 1);

    // TODO: use poll() or epoll() instead?
    auto ret = ::select(nfds, &readfds, nullptr, nullptr, nullptr);
    if (ret != -1) {
        if (FD_ISSET(signal_fd, &readfds)) {
            env->raise("IOError", "closed stream");
        } else if (FD_ISSET(thread_signal_fd, &readfds)) {
            thread->clear_signal();
            ThreadObject::cancelation_checkpoint(env);
        }
    }
}

void ThreadObject::cancelation_checkpoint(Env *env) {
    auto thread = current();

    // This call gives us a cancelation point that works with pthread_cancel(3).
    ::usleep(0);

    if (thread->m_exception) {
        auto exception = Value(thread->m_exception).send(env, "exception"_s, {})->as_exception_or_raise(env);
        thread->set_exception(nullptr);
        env->raise_exception(exception);
    }
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
