#include "natalie.hpp"

namespace Natalie {

void Constant::autoload(Env *env, Value self) {
    if (m_autoload_block) {
        auto block = m_autoload_block;
        m_autoload_block = nullptr;
        block->run(env);
        return;
    }
    assert(m_autoload_fn);
    auto fn = m_autoload_fn;
    m_autoload_fn = nullptr; // remove autoload fn so we don't have a possible loop
    fn(env, self, {}, nullptr);
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
