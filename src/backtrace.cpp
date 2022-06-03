#include "natalie.hpp"

namespace Natalie {
ArrayObject *Backtrace::to_ruby_array() {
    ArrayObject *generated = new ArrayObject { m_items.size() };
    for (auto item : m_items) {
        generated->push(StringObject::format("{}:{}:in `{}'", item.file, item.line, item.source_location));
    }
    return generated;
}
}
