#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/forward.hpp"
#include "natalie/macros.hpp"
#include "tm/string.hpp"

namespace Natalie {

class Cell {
public:
    enum class Status {
        Unused, // in the free list, not yet allocated
        Unmarked, // allocated, not yet marked (maybe unreachable)
        Marked, // allocated, reachable
        MarkedAndVisited, // allocated, reachable, and visited already
    };

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
        m_status = Status::MarkedAndVisited;
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
        return m_status == Status::Marked || m_status == Status::MarkedAndVisited;
    }

    bool is_visited() const {
        return m_status == Status::MarkedAndVisited;
    }

    void mark() const {
        m_status = Status::Marked;
    }

    void unmark() const {
        m_status = Status::Unmarked;
    }

private:
    // The reason that Cells are "Marked" by default is because of an insidious race condition
    // in multi-threaded code that goes something like this:
    //
    // 1. Thread A allocates a new Cell, let's pretend it's a StringObject "Jimmy".
    //    A new Cell is allocated, but before the object initialization is complete...
    // 2. Thread Main pauses the world and starts marking reachable Cells.
    // 3. Thread Main does indeed see that "Jimmy" is reachable, and marks him.
    // 4. Thread Main wakes up the world.
    // 5. Thread A finishes initializing "Jimmy" and default initializes the `m_status` value.
    // 6. Thread Main sweeps unmarked cells.
    //
    // If the default was "Unmarked", then Thread Main would sweep the brand new object
    // that is indeed reachable and was just marked. :-/
    //
    // The downside to this is that every Cell will live through at least one GC cycle.
    // We may come up with another solution down the road...
    mutable Status m_status { Status::Marked };
};

}
