#pragma once

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {
class GlobalVariableInfo : public Cell {
public:
    using assignment_hook_t = void (*)(Env *, Object **, Object *);

    GlobalVariableInfo(Object *object, bool readonly)
        : m_object { object }
        , m_readonly { readonly } { }

    void set_object(Env *, Object *);
    Object *object() { return m_object; }
    bool is_readonly() const { return m_readonly; }

    void set_assignment_hook(assignment_hook_t assignment_hook) { m_assignment_hook = assignment_hook; }

    virtual void visit_children(Visitor &visitor) override final;

private:
    class Object *m_object { nullptr };
    bool m_readonly;
    assignment_hook_t m_assignment_hook { nullptr };
};

}
