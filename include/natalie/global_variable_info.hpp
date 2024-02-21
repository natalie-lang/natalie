#pragma once

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {
class GlobalVariableInfo : public Cell {
public:
    GlobalVariableInfo(Object *object, bool readonly)
        : m_object { object }
        , m_readonly { readonly } { }

    void set_object(Object *object) { m_object = object; }
    Object *object() { return m_object; }
    bool is_readonly() const { return m_readonly; }

    virtual void visit_children(Visitor &visitor) override final;

private:
    class Object *m_object { nullptr };
    bool m_readonly;
};

}
