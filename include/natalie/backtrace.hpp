#pragma once

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "tm/hashmap.hpp"

namespace Natalie {
class Backtrace : public Cell {
public:
    struct Item {
        String source_location;
        const char *file;
        size_t line;
    };

    void add_item(Env *env, const char *file, size_t line) {
        // NATFIXME: Ideally we could store the env and delay building the code location name
        m_items.push(Item { env->build_code_location_name(), file, line });
    }
    ArrayObject *to_ruby_array();

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<Backtrace %p size=%ld>", this, m_items.size());
    }

private:
    Vector<Item> m_items;
};

}
