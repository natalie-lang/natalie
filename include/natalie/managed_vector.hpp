#pragma once

#include "natalie/gc.hpp"
#include "tm/vector.hpp"

namespace Natalie {

template <typename T>
class ManagedVector : public Cell, public TM::Vector<T> {
public:
    using TM::Vector<T>::Vector;

    virtual ~ManagedVector() { }

    virtual void visit_children(Visitor &visitor) override final {
        for (auto it = TM::Vector<T>::begin(); it != TM::Vector<T>::end(); ++it) {
            visitor.visit(*it);
        }
    }

    virtual void gc_print() override {
        size_t the_size = TM::Vector<T>::size();
        fprintf(stderr, "<ManagedVector %p size=%zu [", this, the_size);
        size_t index = 0;
        for (auto it = TM::Vector<T>::begin(); it != TM::Vector<T>::end(); ++it) {
            auto item = *it;
            item->gc_print();
            if (index + 1 < the_size)
                fprintf(stderr, ", ");
            ++index;
        }
        fprintf(stderr, "]>");
    }
};

}
