#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/forward.hpp"

namespace Natalie {

class Cell {
public:
    Cell() { }
    virtual ~Cell() { }

    void *operator new(size_t size);
    void operator delete(void *ptr);

    class Visitor {
    public:
        virtual void visit(Cell *) = 0;
        virtual void visit(ValuePtr) = 0;
    };

    void virtual visit_children(Visitor &) = 0;

    bool marked() {
        return m_marked;
    }

    void mark() {
        m_marked = true;
    }

    void unmark() {
        m_marked = false;
    }

private:
    bool m_marked { false };
};

}
