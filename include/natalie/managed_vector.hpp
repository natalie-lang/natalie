#pragma once

#include "gc/gc_cpp.h"
#include "tm/vector.hpp"

namespace Natalie {

template <typename T>
class ManagedVector : public gc, public TM::Vector<T> {
public:
    using TM::Vector<T>::Vector;

    ManagedVector(const TM::Vector<T> &other)
        : ManagedVector {} {
        for (auto item : other)
            this->push(item);
    }

    virtual ~ManagedVector() { }

    virtual void gc_inspect(char *buf, size_t len) const {
        size_t the_size = TM::Vector<T>::size();
        snprintf(buf, len, "<ManagedVector %p size=%zu>", this, the_size);
    }
};

}
