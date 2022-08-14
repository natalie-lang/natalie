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

    MethodVisibility visibility() const { return m_visibility; }

    Method *method() const {
        assert(m_method);
        return m_method;
    }

    operator bool() const { return m_method; }

private:
    MethodVisibility m_visibility { MethodVisibility::Public };
    Method *m_method { nullptr };
};
}
