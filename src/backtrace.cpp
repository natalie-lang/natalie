#include "natalie.hpp"

namespace Natalie {
ArrayObject *Backtrace::to_ruby_array() {
    ArrayObject *generated = ArrayObject::create(m_items.size());
    for (auto item : m_items) {
        generated->push(StringObject::format("{}:{}:in '{}'", item.file, item.line, item.source_location));
    }
    return generated;
}

ArrayObject *Backtrace::to_ruby_backtrace_locations_array() {
    ArrayObject *generated = ArrayObject::create(m_items.size());
    for (auto item : m_items) {
        generated->push(new Thread::Backtrace::LocationObject { item.source_location, item.file, item.line });
    }
    return generated;
}

}
