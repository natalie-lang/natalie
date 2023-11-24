#include <signal.h>

#include "natalie.hpp"

static void set_end_of_stack_for_thread(pthread_t thread_id, Natalie::ThreadObject *thread_object) {
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

extern "C" {

void *nat_create_thread(void *thread_object) {
    auto thread = (Natalie::ThreadObject *)thread_object;
#ifdef __SANITIZE_ADDRESS__
    thread->set_asan_fake_stack(__asan_get_current_fake_stack());
#endif

    auto thread_id = thread->thread_id();
    set_end_of_stack_for_thread(thread_id, thread);

    thread->set_status(Natalie::ThreadObject::Status::Active);

    auto block = thread->block();

    Natalie::Env e {};
    try {
        auto return_value = NAT_RUN_BLOCK_WITHOUT_BREAK((&e), block, Natalie::Args(), nullptr);
        thread->set_status(Natalie::ThreadObject::Status::Terminated);
        return return_value.object();
    } catch (Natalie::ExceptionObject *exception) {
        Natalie::handle_top_level_exception(&e, exception, false);
        thread->set_status(Natalie::ThreadObject::Status::Errored);
        return Natalie::NilObject::the();
    }
}
}

namespace Natalie {

std::mutex g_thread_mutex;

ThreadObject *ThreadObject::current() {
    std::lock_guard<std::mutex> lock(g_thread_mutex);

    auto current = pthread_self();
    for (auto thread : s_list) {
        if (thread->thread_id() == current)
            return thread;
    }

    NAT_UNREACHABLE();
}

void ThreadObject::build_main_thread(void *start_of_stack) {
    assert(!s_main && !s_main_id); // can only be built once
    auto thread = new ThreadObject;
    assert(start_of_stack);
    thread->m_start_of_stack = start_of_stack;
    thread->m_status = ThreadObject::Status::Active;
    thread->m_thread_id = pthread_self();
    set_end_of_stack_for_thread(thread->m_thread_id, thread);
    s_main = thread;
    s_main_id = thread->m_thread_id;
}

ThreadObject *ThreadObject::initialize(Env *env, Block *block) {
    assert(this != ThreadObject::main()); // can never be main thread

    env->ensure_block_given(block);
    m_block = block;

    m_file = env->file();
    m_line = env->line();

    pthread_create(&m_thread_id, nullptr, nat_create_thread, (void *)this);

    return this;
}

Value ThreadObject::status(Env *env) {
    switch (m_status) {
    case Status::Created:
        return new StringObject { "sleep" };
    case Status::Active:
        return new StringObject {
            pthread_self() == m_thread_id ? "run" : "sleep"
        };
    case Status::Errored:
        return NilObject::the();
    case Status::Terminated:
        return FalseObject::the();
    }
    NAT_UNREACHABLE();
}

Value ThreadObject::join(Env *) const {
    void *return_value = nullptr;
    auto result = pthread_join(m_thread_id, &return_value);

    // TODO: handle error more gracefully
    assert(result == 0);

    // TODO
    // return (Object *)return_value;
    return NilObject::the();
}

Value ThreadObject::kill(Env *) {
    m_status = Status::Terminated;
    if (is_main())
        exit(0);
    pthread_kill(m_thread_id, SIGINT);
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

void ThreadObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_block);
    visitor.visit(m_storage);
    visitor.visit(m_exception);
    visit_children_from_stack(visitor);
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
