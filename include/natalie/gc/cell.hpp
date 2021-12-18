#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/forward.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

class Cell {
public:
    Cell() { }
    virtual ~Cell() { }

    void *operator new(size_t size);
    void *operator new(size_t size, void *cell) { return cell; }
    void operator delete(void *ptr);

    class Visitor {
    public:
        virtual void visit(Cell *) = 0;
        virtual void visit(const Cell *) = 0;
        virtual void visit(Value) = 0;
    };

    virtual void visit_children(Visitor &) {
    }

    // only for debugging the GC
    virtual void gc_inspect(char *buf, size_t len) const {
        snprintf(buf, len, "<Cell %p size=%zu>", this, sizeof(*this));
    }

    // only for debugging the GC
    virtual void gc_print() const {
        char buf[1000];
        gc_inspect(buf, 1000);
        printf("%s\n", buf);
    }

    virtual bool is_collectible() {
        return true;
    }

    bool is_marked() const {
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
