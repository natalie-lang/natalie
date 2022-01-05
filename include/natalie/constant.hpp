#pragma once
#include "natalie/forward.hpp"
#include "natalie/value.hpp"

namespace Natalie {
class Constant : public Cell {
public:
    Constant(SymbolObject *name, Value value)
        : m_name(name)
        , m_value(value) {};

    SymbolObject *name() const { return m_name; }
    Value value() const { return m_value; }
    bool is_private() const { return m_private; }
    void set_private(bool is_private) { this->m_private = is_private; }
    bool is_deprecated() const { return m_deprecated; }
    void set_deprecated(bool is_deprecated) { this->m_deprecated = is_deprecated; }

    void visit_children(Visitor &visitor) {
        visitor.visit(m_value);
    }

private:
    SymbolObject *m_name;
    Value m_value;
    bool m_private = false;
    bool m_deprecated = false;
};
}
