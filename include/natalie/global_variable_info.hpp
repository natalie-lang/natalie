#pragma once

#include <functional>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {
class GlobalVariableInfo : public Cell {
public:
    using read_hook_t = std::function<Value(Env *, GlobalVariableInfo &)>;
    using write_hook_t = Object *(*)(Env *, Value, GlobalVariableInfo &);

    GlobalVariableInfo(Object *object, bool readonly)
        : m_object { object }
        , m_readonly { readonly } { }

    void set_object(Env *, Object *);
    Value object(Env *);
    bool is_readonly() const { return m_readonly; }

    void set_read_hook(read_hook_t read_hook) { m_read_hook = read_hook; }
    void set_write_hook(write_hook_t write_hook) { m_write_hook = write_hook; }

    virtual void visit_children(Visitor &visitor) override final;

private:
    class Object *m_object { nullptr };
    bool m_readonly;
    read_hook_t m_read_hook { nullptr };
    write_hook_t m_write_hook { nullptr };
};

}
