#pragma once

#include "natalie/gc.hpp"
#include "tm/vector.hpp"

namespace Natalie {

template <typename T>
class ManagedVector : public Cell, public TM::Vector<T> {
public:
    using TM::Vector<T>::Vector;

    ManagedVector(const Vector<T> &other)
        : ManagedVector {} {
        concat(other);
    }

    virtual ~ManagedVector() { }

    virtual void visit_children(Visitor &visitor) const override final {
        for (auto it = TM::Vector<T>::begin(); it != TM::Vector<T>::end(); ++it) {
            visitor.visit(*it);
        }
    }

    virtual TM::String dbg_inspect() const override {
        size_t the_size = TM::Vector<T>::size();
        return TM::String::format("<ManagedVector {h} size={}>", this, the_size);
    }
};

}
