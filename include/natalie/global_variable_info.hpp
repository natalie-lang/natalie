#pragma once

#include <functional>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/value.hpp"
#include "tm/optional.hpp"

namespace Natalie {
class GlobalVariableInfo : public Cell {
public:
    using read_hook_t = std::function<Optional<Value>(Env *, GlobalVariableInfo &)>;
    using write_hook_t = Value (*)(Env *, Value, GlobalVariableInfo &);

    GlobalVariableInfo(Optional<Value> value, bool readonly)
        : m_value { value }
        , m_readonly { readonly } {
    }

    void set_object(Env *, Value);
    Optional<Value> object(Env *);
    bool is_readonly() const { return m_readonly; }

    void set_read_hook(read_hook_t read_hook) { m_read_hook = read_hook; }
    void set_write_hook(write_hook_t write_hook) { m_write_hook = write_hook; }

    virtual void visit_children(Visitor &visitor) const override final;

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<GlobalVariableInfo {h} value={}>", this, m_value ? m_value.value().dbg_inspect() : "none");
    }

private:
    Optional<Value> m_value {};
    bool m_readonly;
    read_hook_t m_read_hook { nullptr };
    write_hook_t m_write_hook { nullptr };
};

}
