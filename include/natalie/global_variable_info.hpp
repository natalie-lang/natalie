#pragma once

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {
class GlobalVariableInfo : public Cell {
public:
    GlobalVariableInfo(Object *object)
        : m_object { object } { }

    void set_object(Object *object) { m_object = object; }
    Object *object() { return m_object; }

    virtual void visit_children(Visitor &visitor) override final;

private:
    class Object *m_object { nullptr };
};

}
