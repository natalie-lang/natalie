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

    static TimeObject *at(Env *, Value, Value, Value);
    static TimeObject *create(Env *);
    static TimeObject *local(Env *, Value, Value, Value, Value, Value, Value, Value);
    static TimeObject *now(Env *, Value in);
    static TimeObject *utc(Env *, Value, Value, Value, Value, Value, Value, Value);

    Value add(Env *, Value);
    Value asctime(Env *);
    Value cmp(Env *, Value);
    bool eql(Env *, Value);
    Value hour(Env *) const;
    Value inspect(Env *);
    bool is_utc(Env *) const { return m_mode == Mode::UTC; }
    Value mday(Env *) const;
    Value min(Env *) const;
    Value minus(Env *, Value);
    Value month(Env *) const;
    Value nsec(Env *);
    Value sec(Env *) const;
    Value strftime(Env *, Value);
    Value subsec(Env *);
    Value to_f(Env *);
    Value to_i(Env *) const { return m_integer; }
    Value to_r(Env *);
    Value to_s(Env *);
    Value usec(Env *);
    Value utc_offset(Env *) const;
    Value wday(Env *) const;
    Value yday(Env *) const;
    Value year(Env *) const;

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_integer);
        visitor.visit(m_subsec);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<TimeObject %p>", this);
    }

private:
    static RationalObject *convert_rational(Env *, Value);
    static Value convert_unit(Env *, Value);
    static TimeObject *create(Env *, RationalObject *, Mode);

    Value build_string(Env *, const char *);
    void build_time(Env *, Value, Value, Value, Value, Value, Value);
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
