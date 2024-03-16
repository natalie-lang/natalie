#include "natalie/thread_group_object.hpp"

namespace Natalie {

void ThreadGroupObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_default);
}

void ThreadGroupObject::initialize_default() {
    assert(!m_default);
    m_default = new ThreadGroupObject {};
}

}
