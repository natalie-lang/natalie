#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/forward.hpp"
#include "natalie/macros.hpp"
#include "tm/string.hpp"

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
        virtual void visit(Value) = 0;
        virtual void visit(const Cell *) = 0;
    };

    virtual void visit_children(Visitor &) const {
    }

    virtual TM::String dbg_inspect(int indent = 0) const {
        return TM::String::format("<Cell {h} size={}>", this, sizeof(*this));
    }

    virtual bool is_collectible() {
        return true;
    }

    virtual bool is_large() {
        return false;
    }

    bool is_marked() const {
        return m_marked;
    }

    void mark() const {
        m_marked = true;
    }

    void unmark() const {
        m_marked = false;
    }

private:
    mutable bool m_marked { false };
};

}
