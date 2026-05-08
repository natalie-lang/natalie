#include "natalie.hpp"

namespace Natalie {

IClassObject::IClassObject(ModuleObject *wrapped, ClassObject *super)
    : ClassObject { reinterpret_cast<ClassObject *>(-1) }
    , m_wrapped_module { wrapped } {
    // We avoid going through GlobalEnv::the()->Class() because iclasses can be
    // allocated during early bootstrap before Class is wired up. The bogus -1
    // klass marker is replaced below if Class is available.
    m_klass = GlobalEnv::the() ? GlobalEnv::the()->Class() : nullptr;
    set_superclass_DANGEROUSLY(super);
}

IClassObject *IClassObject::create(ModuleObject *wrapped, ClassObject *super) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    return new IClassObject(wrapped, super);
}

TM::Hashmap<SymbolObject *, MethodInfo> &IClassObject::methods_table() {
    return m_wrapped_module->methods_table();
}

const TM::Hashmap<SymbolObject *, MethodInfo> &IClassObject::methods_table() const {
    return m_wrapped_module->methods_table();
}

TM::Hashmap<SymbolObject *, Constant *> &IClassObject::constants_table() {
    return m_wrapped_module->constants_table();
}

const TM::Hashmap<SymbolObject *, Constant *> &IClassObject::constants_table() const {
    return m_wrapped_module->constants_table();
}

TM::Hashmap<SymbolObject *, Optional<Value>> &IClassObject::class_vars_table() {
    return m_wrapped_module->class_vars_table();
}

const TM::Hashmap<SymbolObject *, Optional<Value>> &IClassObject::class_vars_table() const {
    return m_wrapped_module->class_vars_table();
}

void IClassObject::visit_children(Visitor &visitor) const {
    ClassObject::visit_children(visitor);
    visitor.visit(m_wrapped_module);
    visitor.visit(m_includer);
}

TM::String IClassObject::dbg_inspect(int) const {
    auto wrapped_name = m_wrapped_module ? m_wrapped_module->name().value_or("none") : "<null>";
    return TM::String::format("<IClassObject {h} wrapping=\"{}\">", this, wrapped_name);
}

}
