#pragma once

#include "natalie/gc.hpp"
#include "natalie/method_visibility.hpp"

namespace Natalie {

class MethodInfo {
public:
    MethodInfo() { }

    MethodInfo(MethodVisibility visibility, Method *method)
        : m_visibility { visibility }
        , m_method { method } {
        assert(m_method);
    }

    MethodInfo(MethodVisibility visibility)
        : m_visibility { visibility }
        , m_undefined { true } { }

    MethodVisibility visibility() const { return m_visibility; }

    Method *method() const {
        assert(m_method);
        return m_method;
    }

    operator bool() const { return m_method || m_undefined; }

    bool is_defined() const { return !m_undefined && m_method; }

    void visit_children(Cell::Visitor &);

private:
    MethodVisibility m_visibility { MethodVisibility::Public };
    Method *m_method { nullptr };
    bool m_undefined { false };
};
}
