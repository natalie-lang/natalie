#include "natalie.hpp"

namespace Natalie {

TimeObject *TimeObject::at(Env *env, Value time, Value subsec) {
    RationalObject *rational;
    if (time->is_integer()) {
        rational = RationalObject::create(env, time->as_integer(), new IntegerObject { 1 });
    } else if (time->is_rational()) {
        rational = time->as_rational();
    } else {
        // NATFIXME: Support other arguments (Float, BigDecimal, Time etc)
        env->raise("TypeError", "can't convert {} into an exact number", time->klass()->inspect_str());
    }
    if (subsec) {
        if (subsec->is_integer()) {
            IntegerObject *integer = subsec->as_integer();
            if (integer->lt(env, new IntegerObject { 0 }) || integer->gte(env, new IntegerObject { 1000000 })) {
                env->raise("ArgumentError", "subsecx out of range");
            }
            rational = rational->add(env, RationalObject::create(env, integer, new IntegerObject { 1000000 }))->as_rational();
        } else if (subsec->is_rational()) {
            rational = rational->add(env, subsec->as_rational()->div(env, new IntegerObject { 1000000 }))->as_rational();
        } else {
            env->raise("TypeError", "can't convert {} into an exact number", subsec->klass()->inspect_str());
        }
    }
    return TimeObject::create(env, rational, Mode::Localtime);
}

TimeObject *TimeObject::local(Env *env, Value year, Value month, Value mday, Value hour, Value min, Value sec, Value usec) {
    struct tm time = build_time_struct(env, year, month, mday, hour, min, sec);
    int seconds = mktime(&time);
    TimeObject *result = new TimeObject {};
    result->m_time = time;
    result->m_mode = Mode::Localtime;
    result->m_integer = Value::integer(seconds);
    if (usec && usec->is_integer()) {
        result->set_subsec(env, usec->as_integer());
    }
    return result;
}

TimeObject *TimeObject::create(Env *env) {
    return now(env);
}

TimeObject *TimeObject::now(Env *env) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    struct tm time = *localtime(&ts.tv_sec);
    TimeObject *result = new TimeObject {};
    result->m_time = time;
    result->m_mode = Mode::Localtime;
    result->m_integer = Value::integer(ts.tv_sec);
    result->set_subsec(env, ts.tv_nsec);
    return result;
}

TimeObject *TimeObject::utc(Env *env, Value year, Value month, Value mday, Value hour, Value min, Value sec, Value subsec) {
    struct tm time = build_time_struct(env, year, month, mday, hour, min, sec);
    time.tm_gmtoff = 0;
    int seconds = timegm(&time);
    TimeObject *result = new TimeObject {};
    result->m_time = time;
    result->m_mode = Mode::UTC;
    result->m_integer = Value::integer(seconds);
    if (subsec) {
        if (subsec->is_integer()) {
            IntegerObject *integer = subsec->as_integer();
            if (integer->lt(env, new IntegerObject { 0 }) || integer->gte(env, new IntegerObject { 1000000 })) {
                env->raise("ArgumentError", "subsecx out of range");
            }
            result->m_subsec = RationalObject::create(env, integer, new IntegerObject { 1000000 });
        } else if (subsec->is_rational()) {
            result->m_subsec = subsec->as_rational()->div(env, new IntegerObject { 1000000 });
        } else {
            env->raise("TypeError", "can't convert {} into an exact number", subsec->klass()->inspect_str());
        }
    }
    return result;
}

Value TimeObject::add(Env *env, Value other) {
    if (other->is_time()) {
        env->raise("TypeError", "time + time?");
    } else if (other->is_nil() || !other->respond_to(env, "to_r"_s)) {
        env->raise("TypeError", "can't convert {} into an exact number", other->klass()->inspect_str());
    }
    RationalObject *rational = to_r(env)->as_rational();
    rational = rational->add(env, other->send(env, "to_r"_s)->as_rational())->as_rational();
    return TimeObject::create(env, rational, m_mode);
}

Value TimeObject::asctime(Env *env) {
    return build_string(env, "%a %b %e %T %Y");
}

Value TimeObject::cmp(Env *env, Value other) {
    if (other->is_time()) {
        auto time = other->as_time();
        auto integer = m_integer->as_integer();
        if (integer->gt(env, time->m_integer)) {
            return Value::integer(1);
        } else if (integer->lt(env, time->m_integer)) {
            return Value::integer(-1);
        } else {
            return subsec(env)->send(env, "to_r"_s)->as_rational()->cmp(env, time->subsec(env)->send(env, "to_r"_s));
        }
    } else {
        auto result = other->send(env, "<=>"_s, { this });
        if (result->is_nil()) {
            return result;
        } else {
            if (result->send(env, ">"_s, { Value::integer(0) })->is_true()) {
                return Value::integer(-1);
            } else if (result->send(env, "<"_s, { Value::integer(0) })->is_true()) {
                return Value::integer(1);
            } else {
                return Value::integer(0);
            }
        }
    }
}

bool TimeObject::eql(Env *env, Value other) {
    if (other->is_time()) {
        auto time = other->as_time();
        if (m_integer->as_integer()->eq(env, time->m_integer->as_integer())) {
            if (m_subsec && time->m_subsec && m_subsec->as_rational()->eq(env, time->m_subsec)) {
                return true;
            } else if (!m_subsec && !time->m_subsec) {
                return true;
            }
        }
    }
    return false;
}

Value TimeObject::hour(Env *) const {
    return Value::integer(m_time.tm_hour);
}

Value TimeObject::inspect(Env *env) {
    StringObject *result = build_string(env, "%Y-%m-%d %H:%M:%S")->as_string();
    if (m_subsec) {
        auto integer = m_subsec->as_rational()->mul(env, new IntegerObject { 1000000000 })->as_rational()->to_i(env);
        auto string = integer->to_s(env);
        auto length = string->length();
        if (length > 9) {
            string->truncate(9);
        } else if (length < 9) {
            nat_int_t n;
            for (n = 9 - length - 1; n >= 0; n--) {
                string->prepend_char(env, '0');
            }
        }
        result->append_char('.');
        result->append(strip_zeroes(string));
    }
    if (is_utc(env)) {
        result->append(" UTC");
    } else {
        char buffer[7];
        auto length = ::strftime(buffer, 7, " %z", &m_time);
        result->append(buffer);
    }
    return result;
}

Value TimeObject::mday(Env *) const {
    return Value::integer(m_time.tm_mday);
}

Value TimeObject::min(Env *) const {
    return Value::integer(m_time.tm_min);
}

Value TimeObject::month(Env *) const {
    return Value::integer(m_time.tm_mon + 1);
}

Value TimeObject::nsec(Env *env) {
    if (m_subsec) {
        return m_subsec->as_rational()->mul(env, new IntegerObject { 1000000000 })->as_rational()->to_i(env);
    } else {
        return Value::integer(0);
    }
}

Value TimeObject::sec(Env *) const {
    return Value::integer(m_time.tm_sec);
}

Value TimeObject::strftime(Env *env, Value format) {
    return build_string(env, format->as_string()->c_str());
}

Value TimeObject::subsec(Env *) {
    if (m_subsec) {
        return m_subsec;
    } else {
        return Value::integer(0);
    }
}

Value TimeObject::to_f(Env *env) {
    Value result = m_integer->as_integer()->to_f();
    if (m_subsec) {
        result = result->as_float()->add(env, m_subsec->as_rational());
    }
    return result;
}

Value TimeObject::to_r(Env *env) {
    Value result = RationalObject::create(env, m_integer->as_integer(), new IntegerObject { 1 });
    if (m_subsec) {
        result = result->as_rational()->add(env, m_subsec->as_rational());
    }
    return result;
}

Value TimeObject::to_s(Env *env) {
    if (is_utc(env)) {
        return build_string(env, "%Y-%m-%d %H:%M:%S UTC");
    } else {
        return build_string(env, "%Y-%m-%d %H:%M:%S %z");
    }
}

Value TimeObject::usec(Env *env) {
    if (m_subsec) {
        return m_subsec->as_rational()->mul(env, new IntegerObject { 1000000 })->as_rational()->to_i(env);
    } else {
        return Value::integer(0);
    }
}

Value TimeObject::wday(Env *) const {
    return Value::integer(m_time.tm_wday);
}

Value TimeObject::yday(Env *) const {
    return Value::integer(m_time.tm_yday + 1);
}

Value TimeObject::year(Env *) const {
    return Value::integer(m_time.tm_year + 1900);
}

struct tm TimeObject::build_time_struct(Env *, Value year, Value month, Value mday, Value hour, Value min, Value sec) {
    struct tm time = { 0 };
    time.tm_year = year->as_integer()->to_nat_int_t() - 1900;
    time.tm_mon = 0;
    time.tm_mday = 1;
    time.tm_hour = 0;
    time.tm_min = 0;
    time.tm_sec = 0;
    if (month && month->is_integer())
        time.tm_mon = month->as_integer()->to_nat_int_t() - 1;
    if (mday && mday->is_integer())
        time.tm_mday = mday->as_integer()->to_nat_int_t();
    if (hour && hour->is_integer())
        time.tm_hour = hour->as_integer()->to_nat_int_t();
    if (min && min->is_integer())
        time.tm_min = min->as_integer()->to_nat_int_t();
    if (sec && sec->is_integer())
        time.tm_sec = sec->as_integer()->to_nat_int_t();
    time.tm_isdst = -1;
    return time;
}

TimeObject *TimeObject::create(Env *env, RationalObject *rational, Mode mode) {
    TimeObject *result = new TimeObject {};
    IntegerObject *integer = rational->to_i(env)->as_integer();
    RationalObject *subseconds = rational->sub(env, integer)->as_rational();
    time_t seconds = (time_t)integer->to_nat_int_t();
    if (mode == Mode::UTC) {
        result->m_time = *gmtime(&seconds);
    } else {
        result->m_time = *localtime(&seconds);
    }
    result->m_mode = mode;
    result->m_integer = integer;
    result->set_subsec(env, subseconds);
    return result;
}

void TimeObject::set_subsec(Env *env, long nsec) {
    if (nsec > 0) {
        m_subsec = RationalObject::create(env, new IntegerObject { nsec }, new IntegerObject { 1000000000 });
    }
}

void TimeObject::set_subsec(Env *env, IntegerObject *usec) {
    if (usec->lt(env, new IntegerObject { 0 }) || usec->gte(env, new IntegerObject { 1000000 })) {
        env->raise("ArgumentError", "subsecx out of range");
    }
    if (!usec->is_zero()) {
        m_subsec = RationalObject::create(env, usec, new IntegerObject { 1000000 });
    }
}

void TimeObject::set_subsec(Env *, RationalObject *subsec) {
    if (!subsec->is_zero()) {
        m_subsec = subsec;
    }
}

Value TimeObject::build_string(Env *, const char *format) {
    int maxsize = 32;
    char buffer[maxsize];
    auto length = ::strftime(buffer, maxsize, format, &m_time);
    return new StringObject { buffer, length };
}

Value TimeObject::strip_zeroes(StringObject *string) {
    nat_int_t last_char;
    nat_int_t len = static_cast<nat_int_t>(string->length());
    const char *c_str = string->c_str();
    for (last_char = len - 1; last_char >= 0; last_char--) {
        char c = c_str[last_char];
        if (c != '0')
            break;
    }
    if (last_char < 0) {
        return new StringObject {};
    } else {
        size_t new_length = static_cast<size_t>(last_char + 1);
        return new StringObject { c_str, new_length };
    }
}

}
