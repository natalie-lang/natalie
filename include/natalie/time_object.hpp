#pragma once

#include <time.h>

#include "natalie/forward.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class TimeObject : public Object {
public:
    enum class Mode {
        Localtime,
        UTC
    };

    TimeObject()
        : Object { Object::Type::Time, GlobalEnv::the()->Time() } { }

    TimeObject(ClassObject *klass)
        : Object { Object::Type::Time, klass } { }

    static TimeObject *at(Env *, Value, Value);
    static TimeObject *create(Env *);
    static TimeObject *local(Env *, Value, Value, Value, Value, Value, Value, Value);
    static TimeObject *now(Env *);
    static TimeObject *utc(Env *, Value, Value, Value, Value, Value, Value, Value);

    Value asctime(Env *);
    bool eql(Env *, Value);
    Value hour(Env *);
    Value inspect(Env *);
    bool is_utc(Env *) const { return m_mode == Mode::UTC; }
    Value mday(Env *);
    Value min(Env *);
    Value month(Env *);
    Value nsec(Env *);
    Value sec(Env *);
    Value strftime(Env *, Value);
    Value subsec(Env *);
    Value to_f(Env *);
    Value to_i(Env *) const { return m_integer; }
    Value to_r(Env *);
    Value to_s(Env *);
    Value usec(Env *);
    Value wday(Env *);
    Value yday(Env *);
    Value year(Env *);

private:
    static struct tm build_time_struct(Env *, Value, Value, Value, Value, Value, Value);

    Value build_string(Env *, const char *);
    Value inspect_usec(Env *);
    void set_subsec(Env *, long);
    void set_subsec(Env *, IntegerObject *);
    void set_subsec(Env *, RationalObject *);
    Value strip_zeroes(StringObject *);

    Value m_integer;
    Mode m_mode;
    Value m_subsec;
    struct tm m_time;
};

}
