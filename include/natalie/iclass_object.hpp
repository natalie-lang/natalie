#pragma once

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"

namespace Natalie {

// IClassObject is a thin proxy node spliced into a class's superclass chain
// when a module is `include`d, `prepend`ed, or `extend`ed. It shares the
// wrapped module's method, constant, and class-variable tables so that later
// `def`s on the module are visible to every including class. IClasses are an
// internal implementation detail and must never leak to user-visible code:
// `defined_class()` always reports the wrapped module instead.
class IClassObject : public ClassObject {
public:
    static IClassObject *create(ModuleObject *wrapped, ClassObject *super);

    bool is_iclass() const override { return true; }

    ModuleObject *wrapped_module() const { return m_wrapped_module; }
    ModuleObject *defined_class() override { return m_wrapped_module; }

    // The class or module whose super chain contains this iclass. Used to
    // propagate late includes/prepends back into includers.
    ModuleObject *includer() const { return m_includer; }
    void set_includer(ModuleObject *includer) { m_includer = includer; }

    // The origin iclass for prepend wraps a real class (the class whose own
    // methods got displaced by the prepend). All other iclasses wrap a Module.
    bool is_origin_iclass() const {
        return m_wrapped_module && m_wrapped_module->type() == Object::Type::Class;
    }

    TM::Hashmap<SymbolObject *, MethodInfo> &methods_table() override;
    const TM::Hashmap<SymbolObject *, MethodInfo> &methods_table() const override;
    TM::Hashmap<SymbolObject *, Constant *> &constants_table() override;
    const TM::Hashmap<SymbolObject *, Constant *> &constants_table() const override;
    TM::Hashmap<SymbolObject *, Optional<Value>> &class_vars_table() override;
    const TM::Hashmap<SymbolObject *, Optional<Value>> &class_vars_table() const override;

    void visit_children(Visitor &) const override;

    TM::String dbg_inspect(int indent = 0) const override;

    IClassObject(const IClassObject &) = delete;
    IClassObject &operator=(const IClassObject &) = delete;

private:
    IClassObject(ModuleObject *wrapped, ClassObject *super);

    ModuleObject *m_wrapped_module { nullptr };
    ModuleObject *m_includer { nullptr };
};

}
