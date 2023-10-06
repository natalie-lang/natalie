#pragma once
#include "natalie/forward.hpp"
#include "natalie/value.hpp"

namespace Natalie {
class Constant : public Cell {
public:
    Constant(SymbolObject *name, Value value)
        : m_name(name)
        , m_value(value) {};

    Constant(SymbolObject *name, MethodFnPtr autoload_fn, StringObject *autoload_path)
        : m_name(name)
        , m_autoload_fn(autoload_fn)
        , m_autoload_path(autoload_path) {};

    SymbolObject *name() const { return m_name; }

    Value value() const {
        assert(m_value);
        return m_value;
    }

    bool is_private() const { return m_private; }
    void set_private(bool is_private) { this->m_private = is_private; }

    bool is_deprecated() const { return m_deprecated; }
    void set_deprecated(bool is_deprecated) { this->m_deprecated = is_deprecated; }

    MethodFnPtr autoload_fn() const { return m_autoload_fn; }
    void set_autoload_fn(MethodFnPtr fn) { m_autoload_fn = fn; }

    bool needs_load() const { return !m_value && m_autoload_fn; }
    StringObject *autoload_path() const { return m_autoload_path; }
    void autoload(Env *, Value);

    void visit_children(Visitor &visitor);

private:
    SymbolObject *m_name;
    Value m_value { nullptr };
    bool m_private { false };
    bool m_deprecated { false };
    MethodFnPtr m_autoload_fn { nullptr };
    StringObject *m_autoload_path { nullptr };
};
}
