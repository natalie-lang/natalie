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
        for (auto item : other)
            this->push(item);
    }

    virtual ~ManagedVector() { }

    virtual void visit_children(Visitor &visitor) override final {
        for (auto it = TM::Vector<T>::begin(); it != TM::Vector<T>::end(); ++it) {
            visitor.visit(*it);
        }
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        size_t the_size = TM::Vector<T>::size();
        snprintf(buf, len, "<ManagedVector %p size=%zu>", this, the_size);
    }
};

}
