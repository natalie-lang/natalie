#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class Thread::Backtrace::LocationObject : public Object {
public:
    LocationObject()
        : Object { Object::Type::ThreadBacktraceLocation, GlobalEnv::the()->Object()->const_fetch("Thread"_s).as_class()->const_fetch("Backtrace"_s).as_module()->const_fetch("Location"_s).as_class() } { }

    LocationObject(ClassObject *klass)
        : Object { Object::Type::ThreadBacktraceLocation, klass } { }

    LocationObject(const String &source_location, const String &file, const size_t line)
        : Object { Object::Type::ThreadBacktraceLocation, GlobalEnv::the()->Object()->const_fetch("Thread"_s).as_class()->const_fetch("Backtrace"_s).as_module()->const_fetch("Location"_s).as_class() }
        , m_source_location { StringObject::create(source_location) }
        , m_file { StringObject::create(file) }
        , m_line { Value::integer(line) } { }

    Value absolute_path(Env *) const;
    StringObject *inspect(Env *) const;
    Value lineno() const { return m_line; }
    Value path() const { return m_file; }
    StringObject *to_s() const;

    virtual void visit_children(Visitor &) const override;

private:
    StringObject *m_source_location { nullptr };
    StringObject *m_file { nullptr };
    Value m_line {};
};

}
