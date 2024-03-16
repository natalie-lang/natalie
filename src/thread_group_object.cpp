#include "natalie/thread_group_object.hpp"

namespace Natalie {

Value ThreadGroupObject::add(Env *env, Value value) {
    if (!value->is_thread())
        env->raise("TypeError", "wrong argument type {} (expected VM/thread)", value->klass()->inspect_str());
    auto thread = value->as_thread();

    std::unique_lock { m_mutex };
    auto old_thread_group = thread->group();
    if (old_thread_group != this) {
        if (old_thread_group) {
            if (old_thread_group->m_enclosed)
                env->raise("ThreadError", "can't move from the enclosed thread group");
            for (size_t i = 0; i < old_thread_group->m_threads.size(); i++) {
                if (old_thread_group->m_threads[i] == thread) {
                    old_thread_group->m_threads.remove(i);
                    break;
                }
            }
        }
        m_threads.push(thread);
        thread->set_group(this);
    }
    return this;
}

Value ThreadGroupObject::enclose() {
    m_enclosed = true;
    return this;
}

ArrayObject *ThreadGroupObject::list() {
    auto result = new ArrayObject { m_threads.size() };
    for (auto thread : m_threads)
        result->push(thread);
    return result;
}

void ThreadGroupObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_default);
    for (auto thread : m_threads)
        visitor.visit(thread);
}

void ThreadGroupObject::initialize_default() {
    assert(!m_default);
    m_default = new ThreadGroupObject {};
}

}
