#pragma once

#include <functional>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/value.hpp"

namespace Natalie {
class GlobalVariableInfo : public Cell {
public:
    using read_hook_t = std::function<Value(Env *, GlobalVariableInfo &)>;
    using write_hook_t = Value (*)(Env *, Value, GlobalVariableInfo &);

    GlobalVariableInfo(Value value, bool readonly)
        : m_value { value }
        , m_readonly { readonly } { }

    void set_object(Env *, Value);
    Value object(Env *);
    bool is_readonly() const { return m_readonly; }

    void set_read_hook(read_hook_t read_hook) { m_read_hook = read_hook; }
    void set_write_hook(write_hook_t write_hook) { m_write_hook = write_hook; }

    virtual void visit_children(Visitor &visitor) const override final;

private:
    Value m_value { nullptr };
    bool m_readonly;
    read_hook_t m_read_hook { nullptr };
    write_hook_t m_write_hook { nullptr };
};

}
