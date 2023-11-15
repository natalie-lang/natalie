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
#ifdef __SANITIZE_ADDRESS__
    void set_asan_fake_stack(void *ptr) { m_asan_fake_stack = ptr; }
#endif

    void set_status(Status status) { m_status = status; }
    Value status(Env *env);

    Block *block() { return m_block; }

    Value join(Env *);

    Value ref(Env *env, Value key);
    Value refeq(Env *env, Value key, Value value);

    pthread_t thread_id() const { return m_thread; }

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

    static ThreadObject *current() {
        NAT_THREAD_LOCK_GUARD();
        auto current = pthread_self();
        for (auto thread : s_list) {
            if (thread->thread_id() == current)
                return thread;
        }
        NAT_UNREACHABLE();
    }

    static ThreadObject *main() { return s_main; }

    static TM::Vector<ThreadObject *> &list() { return s_list; }

private:
    Block *m_block { nullptr };
    HashObject *m_storage { nullptr };
    void *m_start_of_stack { nullptr };
    void *m_end_of_stack { nullptr };
    pthread_t m_thread {};
#ifdef __SANITIZE_ADDRESS__
    void *m_asan_fake_stack { nullptr };
#endif
    Status m_status { Status::Created };
    TM::Optional<TM::String> m_file {};
    TM::Optional<size_t> m_line {};

    inline static ThreadObject *s_main = nullptr;
    inline static TM::Vector<ThreadObject *> s_list {};
};

}
