#pragma once

#include <string.h>
#include <time.h>

#include "natalie/forward.hpp"
#include "natalie/integer.hpp"
#include "natalie/object.hpp"
#include "tm/optional.hpp"

namespace Natalie {

class TimeObject : public Object {
public:
    enum class Mode {
        Localtime,
        UTC
    };

    static TimeObject *create() {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new TimeObject;
    }

    static TimeObject *create(ClassObject *klass) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new TimeObject { klass };
    }

    static TimeObject *create(const TimeObject &other) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new TimeObject { other };
    }

    ~TimeObject() {
        free(m_zone);
    }

    static TimeObject *at(Env *, ClassObject *, Value, Optional<Value> = {}, Optional<Value> = {});
    static TimeObject *at(Env *, ClassObject *, Value, Optional<Value>, Optional<Value>, Optional<Value> in);
    static TimeObject *create(Env *);
    static TimeObject *initialize(Env *, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value> in);
    static TimeObject *local(Env *, Value, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>);
    static TimeObject *now(Env *, ClassObject *, Optional<Value> in = {});
    static TimeObject *utc(Env *, Value, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>);

    Value add(Env *, Value);
    Value asctime(Env *);
    Value cmp(Env *, Value);
    bool eql(Env *, Value);
    Value hour(Env *) const;
    Value inspect(Env *);
    bool isdst(Env *) const { return m_time.tm_isdst > 0; }
    // NATFIXME: This one is broken, it returns true for `Time.new(2000, 1, 1, 20, 15, 01, 3600)`
    bool is_utc(Env *) const { return m_mode == Mode::UTC; }
    Value mday(Env *) const;
    Value min(Env *) const;
    Value minus(Env *, Value);
    Value month(Env *) const;
    Value nsec(Env *);
    Value sec(Env *) const;
    Value strftime(Env *, Value);
    Value subsec(Env *);
    Value to_a(Env *) const;
    Value to_f(Env *);
    Value to_i(Env *) const { return m_integer; }
    Value to_r(Env *);
    Value to_s(Env *);
    Value to_utc(Env *);
    Value usec(Env *);
    Value utc_offset(Env *) const;
    Value wday(Env *) const;
    Value yday(Env *) const;
    Value year(Env *) const;
    Value zone(Env *) const;

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_integer);
        if (m_subsec)
            visitor.visit(m_subsec.value());
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<TimeObject {h} integer={}>", this, m_integer.to_string());
    }

private:
    TimeObject()
        : Object { Object::Type::Time, GlobalEnv::the()->Time() } { }

    TimeObject(ClassObject *klass)
        : Object { Object::Type::Time, klass } { }

    TimeObject(const TimeObject &other)
        : Object { other }
        , m_integer { other.m_integer }
        , m_mode { other.m_mode }
        , m_subsec { other.m_subsec }
        , m_time { other.m_time } {
        if (other.m_zone) {
            m_zone = strdup(other.m_zone);
            m_time.tm_zone = m_zone;
        }
    }

    static RationalObject *convert_rational(Env *, Value);
    static Value convert_unit(Env *, Value);
    static TimeObject *create(Env *, ClassObject *, RationalObject *, Mode);

    static nat_int_t normalize_month(Env *, Value val);
    static nat_int_t normalize_field(Env *, Value val);
    static nat_int_t normalize_field(Env *, Value val, nat_int_t minval, nat_int_t maxval);
    static nat_int_t normalize_timezone(Env *, Value val);

    Value build_string(Env *, const char *);
    void build_time(Env *, Value, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>, Optional<Value>);
    void set_subsec(Env *, long);
    void set_subsec(Env *, Integer);
    void set_subsec(Env *, RationalObject *);
    Value strip_zeroes(StringObject *);

    Integer m_integer;
    Mode m_mode;
    Optional<Value> m_subsec;
    struct tm m_time;
    char *m_zone { nullptr };
};

}
