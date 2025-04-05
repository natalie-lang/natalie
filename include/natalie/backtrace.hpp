#pragma once

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "tm/hashmap.hpp"

namespace Natalie {
class Backtrace : public Cell {
public:
    struct Item {
        String source_location;
        String file;
        size_t line;
    };

    void add_item(Env *env, String file, size_t line) {
        auto name = env->build_code_location_name();
        m_items.push(Item { name, file, line });
    }
    ArrayObject *to_ruby_array();
    ArrayObject *to_ruby_backtrace_locations_array();

    virtual TM::String dbg_inspect() const override {
        return TM::String::format("<Backtrace {h} size={}>", this, m_items.size());
    }

private:
    Vector<Item> m_items;
};

}
