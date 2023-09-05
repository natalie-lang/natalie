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

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<Backtrace %p size=%ld>", this, m_items.size());
    }

private:
    Vector<Item> m_items;
};

}
