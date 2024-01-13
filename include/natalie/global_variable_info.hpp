#pragma once

#include "natalie/forward.hpp"

namespace Natalie {
class GlobalVariableInfo : public gc {
public:
    GlobalVariableInfo(Object *object)
        : m_object { object } { }

    void set_object(Object *object) { m_object = object; }
    Object *object() { return m_object; }

private:
    class Object *m_object { nullptr };
};

}
