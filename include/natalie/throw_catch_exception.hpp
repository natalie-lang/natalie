#pragma once

#include "natalie/forward.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class ThrowCatchException : public Cell {
public:
    ThrowCatchException(Value name, Optional<Value> value = {})
        : m_name { name }
        , m_value { value } { }

    Value get_name() const { return m_name; }
    Optional<Value> get_value() const { return m_value; }

    virtual void visit_children(Visitor &visitor) const override;

private:
    Value m_name {};
    Optional<Value> m_value {};
};

}
