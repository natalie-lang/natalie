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
        Created, // before thread is started
        Active, // thread is running
        Dead, // thread has exited (or had an exception)
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

    static void build_main_thread(void *start_of_stack);

    ThreadObject *initialize(Env *env, Args args, Block *block);

    void set_start_of_stack(void *ptr) { m_start_of_stack = ptr; }
    void *start_of_stack() { return m_start_of_stack; }

    void set_end_of_stack(void *ptr) { m_end_of_stack = ptr; }
    void *end_of_stack() { return m_end_of_stack; }

#ifdef __SANITIZE_ADDRESS__
    void set_asan_fake_stack(void *ptr) { m_asan_fake_stack = ptr; }
#endif

    void set_status(Status status) { m_status = status; }
    Value status(Env *env);

    void set_exception(ExceptionObject *exception) { m_exception = exception; }
    ExceptionObject *exception() { return m_exception; }

    ArrayObject *args() { return m_args; }
    Block *block() { return m_block; }

    bool is_alive() const {
        return m_status == Status::Active || m_status == Status::Created;
    }

    bool is_main() const {
        return m_thread_id == s_main_id;
    }

    bool is_current() const {
        return m_thread_id == pthread_self();
    }

    Value join(Env *);
    Value kill(Env *) const;
    Value raise(Env *, Value = nullptr, Value = nullptr);
    Value run(Env *);
    Value wakeup(Env *);

    Value sleep(Env *, float);

    void set_value(Value value) { m_value = value; }
    Value value(Env *);

    Value fetch(Env *, Value, Value = nullptr, Block * = nullptr);
    bool has_key(Env *, Value);
    Value keys(Env *);
    Value ref(Env *env, Value key);
    Value refeq(Env *env, Value key, Value value);

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

    static Value pass();

    static ThreadObject *current();
    static ThreadObject *main() { return s_main; }

    static Value stop(Env *);

    static pthread_t main_id() { return s_main_id; }

    static bool i_am_main() { return pthread_self() == s_main_id; }

    static TM::Vector<ThreadObject *> &list() { return s_list; }
    static Value list(Env *);

    static void set_current_sleeping(bool is_sleeping) { current()->set_sleeping(is_sleeping); }

private:
    void wait_until_running() const;

    void visit_children_from_stack(Visitor &) const;
    void visit_children_from_asan_fake_stack(Visitor &, Cell *) const;

    friend FiberObject;

    ArrayObject *m_args { nullptr };
    Block *m_block { nullptr };
    HashObject *m_storage { nullptr };
    void *m_start_of_stack { nullptr };
    void *m_end_of_stack { nullptr };
    pthread_t m_thread_id { 0 };
    std::thread m_thread {};
    ExceptionObject *m_exception { nullptr };
    Value m_value { nullptr };
    FiberObject *m_main_fiber { nullptr };
    FiberObject *m_current_fiber { nullptr };
#ifdef __SANITIZE_ADDRESS__
    void *m_asan_fake_stack { nullptr };
#endif
    std::atomic<Status> m_status { Status::Created };
    std::atomic<bool> m_joined { false };
    TM::Optional<TM::String> m_file {};
    TM::Optional<size_t> m_line {};

    bool m_sleeping { false };
    pthread_cond_t m_sleep_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t m_sleep_lock = PTHREAD_MUTEX_INITIALIZER;

    TM::Hashmap<Thread::MutexObject *> m_mutexes {};

    Value m_fiber_scheduler { nullptr };

    inline static pthread_t s_main_id = 0;
    inline static ThreadObject *s_main = nullptr;
    inline static TM::Vector<ThreadObject *> s_list {};
};

}
