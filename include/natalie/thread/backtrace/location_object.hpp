#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie/thread_object.hpp"

namespace Natalie {

class Thread::Backtrace::LocationObject : public Object {
public:
    LocationObject()
        : Object { Object::Type::ThreadBacktraceLocation, GlobalEnv::the()->Object()->const_fetch("Thread"_s)->const_fetch("Backtrace"_s)->const_fetch("Location"_s)->as_class() } { }

    LocationObject(ClassObject *klass)
        : Object { Object::Type::ThreadBacktraceLocation, klass } { }

    LocationObject(const String &source_location, const String &file, const size_t line)
        : Object { Object::Type::ThreadBacktraceLocation, GlobalEnv::the()->Object()->const_fetch("Thread"_s)->const_fetch("Backtrace"_s)->const_fetch("Location"_s)->as_class() }
        , m_source_location { new StringObject { source_location } }
        , m_file { new StringObject { file } }
        , m_line { new IntegerObject { static_cast<nat_int_t>(line) } } { }

    StringObject *inspect(Env *) const;
    StringObject *to_s() const;

    void visit_children(Visitor &);

private:
    StringObject *m_source_location { nullptr };
    StringObject *m_file { nullptr };
    IntegerObject *m_line { nullptr };
};

}
