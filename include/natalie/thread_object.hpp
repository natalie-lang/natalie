#pragma once

#include "natalie/block.hpp"
#include "natalie/class_object.hpp"
#include "natalie/hash_object.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class ThreadObject : public Object {
public:
    enum class Status {
        Created,
        Active,
        Terminated,
        Errored,
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
        size_t i;
        for (i = 0; i < s_list.size(); ++i) {
            if (s_list.at(i)->thread_id() == thread_id())
                break;
        }
        s_list.remove(i);
    }

    static void build_main_thread(void *start_of_stack);

    ThreadObject *initialize(Env *env, Block *block);

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

    Block *block() { return m_block; }

    bool is_alive() const {
        return m_status == Status::Active || m_status == Status::Created;
    }

    bool is_main() const {
        return m_thread_id == s_main_id;
    }

    Value join(Env *) const;
    Value kill(Env *);
    Value raise(Env *, Value = nullptr, Value = nullptr);
    Value wakeup() { return NilObject::the(); }

    Value ref(Env *env, Value key);
    Value refeq(Env *env, Value key, Value value);

    bool is_sleeping() const { return m_sleeping; }
    void set_sleeping(bool sleeping) { m_sleeping = sleeping; }

    pthread_t thread_id() const { return m_thread_id; }

    virtual void visit_children(Visitor &) override final;
    void visit_children_from_stack(Visitor &) const;
    void visit_children_from_asan_fake_stack(Visitor &, Cell *) const;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(
            buf,
            len,
            "<ThreadObject %p stack=%p..%p>",
            this,
            m_end_of_stack,
            m_start_of_stack);
    }

    static Value pass() { return NilObject::the(); }

    static ThreadObject *current();
    static ThreadObject *main() { return s_main; }

    static pthread_t main_id() { return s_main_id; }

    static bool i_am_main() { return pthread_self() == s_main_id; }

    static TM::Vector<ThreadObject *> &list() { return s_list; }

    static void set_current_sleeping(bool is_sleeping) { current()->set_sleeping(is_sleeping); }

private:
    Block *m_block { nullptr };
    HashObject *m_storage { nullptr };
    void *m_start_of_stack { nullptr };
    void *m_end_of_stack { nullptr };
    pthread_t m_thread_id { 0 };
    ExceptionObject *m_exception { nullptr };
#ifdef __SANITIZE_ADDRESS__
    void *m_asan_fake_stack { nullptr };
#endif
    Status m_status { Status::Created };
    TM::Optional<TM::String> m_file {};
    TM::Optional<size_t> m_line {};
    bool m_sleeping { false };

    inline static pthread_t s_main_id = 0;
    inline static ThreadObject *s_main = nullptr;
    inline static TM::Vector<ThreadObject *> s_list {};
};

}