#include "natalie.hpp"

namespace Natalie {

void Constant::autoload(Env *env, Value self, ModuleObject *owner) {
    // Removing the registration prevents recursive lookup from re-entering it.
    // Keep the callback intact so a failed load can restore the registration.
    owner->remove_const(m_name);
    try {
        if (m_autoload_block)
            m_autoload_block->run(env);
        else {
            assert(m_autoload_fn);
            m_autoload_fn(env, self, {}, nullptr);
        }
    } catch (...) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        // MRI rolls back the autoloaded value if its file raises, even when
        // assignment happened before the exception. A replacement autoload
        // registration is distinct from such a provisional value.
        auto replacement = owner->get_constant(m_name, nullptr);
        if (!replacement || replacement->value())
            owner->constants_table().put(m_name, this);
        throw;
    }
}

void Constant::visit_children(Visitor &visitor) const {
    visitor.visit(m_name);
    if (m_value)
        visitor.visit(m_value.value());
    visitor.visit(m_autoload_path);
    visitor.visit(m_autoload_block);
}

TM::String Constant::dbg_inspect(int) const {
    return TM::String::format("<Constant {h} name={}>", this, m_name ? m_name->string() : "null");
}

}
