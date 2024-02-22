#pragma once

#include "natalie/forward.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class ThrowCatchException : public Cell {
public:
    ThrowCatchException(Value name, Value value = nullptr)
        : m_name { name }
        , m_value { value } { }

    Value get_name() const { return m_name; }
    Value get_value() const { return m_value; }

    void visit_children(Visitor &visitor);

private:
    Value m_name { nullptr };
    Value m_value { nullptr };
};

}
