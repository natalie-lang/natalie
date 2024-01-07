#pragma once

#include "natalie/block.hpp"
#include "natalie/class_object.hpp"
#include "natalie/hash_object.hpp"
#include "natalie/nil_object.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

#include <atomic>
#include <thread>

namespace Natalie {

class ThreadObject : public Object {
public:
    enum class Status {
        // 'Created' is an internal state we use to track a thread
        // that is just starting up. We may need to wait a bit
        // before manipulating such a thread, thus you will see
        // wait_until_running() called in a few places.
        Created,

        // 'Active' means the thread is indeed running. Yay!
        Active,

        // 'Aborting' is set by Thread#kill. It means the thread
        // is about to die.
        Aborting,

        // 'Dead' means the thread is well and truly dead.
        // It could have exited naturally, or had an unhandled exception,
        // or it could have been killed.
        Dead,
    };

    ThreadObject()
        : Object { Object::Type::Thread, GlobalEnv::the()->Object()->const_fetch("Thread"_s)->as_class() } {
        s_list.push(this);
    }

    ThreadObject(ClassObject *klass)
        : Object { Object::Type::Thread, klass } {
        s_list.push(this);
    }

    ThreadObject(ClassObject *klass, Block *block)
        : Object { Object::Type::Thread, klass } {
        s_list.push(this);
    }

    virtual ~ThreadObject() {
        // In normal operation, this destructor should only be called for threads that
        // are well and truly dead. (See the is_collectible function.) But in one case --
        // when you compile with the flag NAT_GC_COLLECT_ALL_AT_EXIT -- the GC will
        // destroy every object at the end of the program, even threads that could still
        // be running. In order to be able to safely destroy the std::thread object, we
        // need to detatch any running system thread from the object. And in many cases,
        // the thread will already be joined, so this could error. But we don't care
        // about the error.
        try {
            m_thread.detach();
        } catch (...) { }
    }

    static void build_main_thread(Env *env, void *start_of_stack);

    ThreadObject *initialize(Env *env, Args args, Block *block);

    Value to_s(Env *);

    void set_start_of_stack(void *ptr) { m_start_of_stack = ptr; }
    void *start_of_stack() { return m_start_of_stack; }

    void set_end_of_stack(void *ptr) { m_end_of_stack = ptr; }
    void *end_of_stack() { return m_end_of_stack; }

#ifdef __SANITIZE_ADDRESS__
    void set_asan_fake_stack(void *ptr) { m_asan_fake_stack = ptr; }
#endif

    void set_status(Status status) { m_status = status; }
    Value status(Env *env);
    String status();

    void set_exception(ExceptionObject *exception) { m_exception = exception; }
    ExceptionObject *exception() { return m_exception; }

    ArrayObject *args() { return m_args; }
    Block *block() { return m_block; }

    bool is_alive() const {
        return m_status == Status::Active || m_status == Status::Created || m_status == Status::Aborting;
    }

    bool is_main() const {
        return m_thread_id == s_main_id;
    }

    bool is_current() const {
        return m_thread_id == pthread_self();
    }

    Value join(Env *);
    static Value exit(Env *env) { return current()->kill(env); }
    Value kill(Env *);
    Value raise(Env *, Args args);
    Value run(Env *);
    Value wakeup(Env *);

    Value sleep(Env *, float);

    void set_value(Value value) { m_value = value; }
    Value value(Env *);

    Value name(Env *);
    Value set_name(Env *, Value);

    Value fetch(Env *, Value, Value = nullptr, Block * = nullptr);
    bool has_key(Env *, Value);
    Value keys(Env *);
    Value ref(Env *env, Value key);
    Value refeq(Env *env, Value key, Value value);

    bool has_thread_variable(Env *, Value) const;
    Value thread_variable_get(Env *, Value);
    Value thread_variable_set(Env *, Value, Value);
    Value thread_variables(Env *) const;

    void set_sleeping(bool sleeping) { m_sleeping = sleeping; }
    bool is_sleeping() const { return m_sleeping; }

    bool is_stopped() const;

    void set_thread_id(pthread_t thread_id) { m_thread_id = thread_id; }
    pthread_t thread_id() const { return m_thread_id; }

    void build_main_fiber();
    FiberObject *main_fiber() { return m_main_fiber; }
    FiberObject *current_fiber() { return m_current_fiber; }

    void remove_from_list() const;

    virtual bool is_collectible() override {
        return m_status == Status::Dead && !m_thread.joinable();
    }

    void add_mutex(Thread::MutexObject *mutex);
    void remove_mutex(Thread::MutexObject *mutex);

    void unlock_mutexes() const;

    Value fiber_scheduler() const { return m_fiber_scheduler; }
    void set_fiber_scheduler(Value scheduler) { m_fiber_scheduler = scheduler; }

    bool abort_on_exception() const { return m_abort_on_exception; }
    bool set_abort_on_exception(bool abrt) {
        m_abort_on_exception = abrt;
        return abrt;
    }
    bool set_abort_on_exception(Value abrt) {
        m_abort_on_exception = abrt->is_truthy();
        return abrt;
    }

    bool report_on_exception() const { return m_report_on_exception; }
    bool set_report_on_exception(bool report) {
        m_report_on_exception = report;
        return report;
    }
    bool set_report_on_exception(Value report) {
        m_report_on_exception = report->is_truthy();
        return report;
    }

    void check_exception(Env *);

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(
            buf,
            len,
            "<ThreadObject %p stack=%p..%p>",
            this,
            m_end_of_stack,
            m_start_of_stack);
    }

    static Value pass(Env *env);

    static ThreadObject *current();
    static ThreadObject *main() { return s_main; }

    static Value stop(Env *);

    static pthread_t main_id() { return s_main_id; }

    static bool i_am_main() { return pthread_self() == s_main_id; }

    static TM::Vector<ThreadObject *> &list() { return s_list; }
    static Value list(Env *);

    static void set_current_sleeping(bool is_sleeping) { current()->set_sleeping(is_sleeping); }

    static bool default_report_on_exception() { return s_report_on_exception; }
    static bool set_default_report_on_exception(bool report) {
        s_report_on_exception = report;
        return report;
    }
    static bool set_default_report_on_exception(Value report) {
        s_report_on_exception = report->is_truthy();
        return report;
    }

    static bool global_abort_on_exception() { return s_abort_on_exception; }
    static bool set_global_abort_on_exception(bool abrt) {
        s_abort_on_exception = abrt;
        return abrt;
    }
    static bool set_global_abort_on_exception(Value abrt) {
        s_abort_on_exception = abrt->is_truthy();
        return abrt;
    }

    static void check_current_exception(Env *env);

    static void setup_interrupt_pipe(Env *env);
    static int interrupt_read_fileno() { return s_interrupt_read_fileno; }
    static void interrupt();
    static void clear_interrupt();

    static ClassObject *thread_kill_class() { return s_thread_kill_class; }
    static ClassObject *thread_kill_class(Env *env) {
        if (!s_thread_kill_class)
            s_thread_kill_class = GlobalEnv::the()->Object()->subclass(env, "ThreadKillError");
        return s_thread_kill_class;
    }

private:
    void wait_until_running() const;

    void visit_children_from_stack(Visitor &) const;
    void visit_children_from_asan_fake_stack(Visitor &, Cell *) const;

    friend FiberObject;

    ArrayObject *m_args { nullptr };
    Block *m_block { nullptr };
    void *m_start_of_stack { nullptr };
    void *m_end_of_stack { nullptr };
    pthread_t m_thread_id { 0 };
    std::thread m_thread {};
    std::atomic<ExceptionObject *> m_exception { nullptr };
    Value m_value { nullptr };
    HashObject *m_thread_variables { nullptr };
    FiberObject *m_main_fiber { nullptr };
    FiberObject *m_current_fiber { nullptr };
#ifdef __SANITIZE_ADDRESS__
    void *m_asan_fake_stack { nullptr };
#endif
    std::atomic<Status> m_status { Status::Created };
    std::atomic<bool> m_joined { false };
    TM::Optional<TM::String> m_name {};
    TM::Optional<TM::String> m_file {};
    TM::Optional<size_t> m_line {};

    bool m_abort_on_exception { false };
    bool m_report_on_exception { true };

    bool m_sleeping { false };

    TM::Hashmap<Thread::MutexObject *> m_mutexes {};

    Value m_fiber_scheduler { nullptr };

    // This condition variable is used to wake a sleeping thread,
    // i.e. a thread where Kernel#sleep or Thread#sleep has been called.
    pthread_cond_t m_sleep_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t m_sleep_lock = PTHREAD_MUTEX_INITIALIZER;

    inline static pthread_t s_main_id = 0;
    inline static ThreadObject *s_main = nullptr;
    inline static TM::Vector<ThreadObject *> s_list {};
    inline static bool s_abort_on_exception { false };
    inline static bool s_report_on_exception { true };

    // In addition to m_sleep_cond which can wake a sleeping thread,
    // we also need a way to wake a thread that is blocked on reading
    // from a file descriptor. Any time select(2) is called, we can
    // add this s_interrupt_read_fileno to the fd_set so we have
    // a way to unblock the call. This is also signaled when any
    // IO object is closed, since we'll need to wake up any blocking
    // select() calls and check if the IO object was closed.
    // TODO: we'll need to rebuild these after a fork :-/
    inline static int s_interrupt_read_fileno { -1 };
    inline static int s_interrupt_write_fileno { -1 };

    // We use this special class as an off-the-books exception class
    // for killing threads. It cannot be rescued in user code, but it
    // does trigger `ensure` blocks.
    // We only build this class once it is needed.
    inline static ClassObject *s_thread_kill_class { nullptr };
};

}
