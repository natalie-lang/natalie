#pragma once

#include "natalie/gc.hpp"
#include "natalie/method_visibility.hpp"

namespace Natalie {

class MethodInfo : public Cell {
public:
    MethodInfo(const char *name, MethodVisibility visibility)
        : m_name { name }
        , m_visibility { visibility } { }

    MethodVisibility visibility() { return m_visibility; }
    void set_visibility(MethodVisibility visibility) { m_visibility = visibility; }

    virtual void visit_children(Visitor &visitor) override final { }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<MethodInfo %p name='%s'>", this, m_name);
    }

private:
    const char *m_name {};
    MethodVisibility m_visibility { MethodVisibility::Public };
};
}