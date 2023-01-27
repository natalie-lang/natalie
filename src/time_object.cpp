#include "natalie.hpp"

namespace Natalie {

TimeObject *TimeObject::at(Env *env, Value time, Value subsec, Value unit) {
    RationalObject *rational = convert_rational(env, time);
    if (subsec) {
        auto scale = convert_unit(env, unit ? unit : "microsecond"_s);
        rational = rational->add(env, convert_rational(env, subsec)->div(env, scale))->as_rational();
    }
    return TimeObject::create(env, rational, Mode::Localtime);
}

TimeObject *TimeObject::local(Env *env, Value year, Value month, Value mday, Value hour, Value min, Value sec, Value usec) {
    TimeObject *result = new TimeObject {};
    result->build_time(env, year, month, mday, hour, min, sec);
    int seconds = mktime(&result->m_time);
    result->m_mode = Mode::Localtime;
    result->m_integer = Value::integer(seconds);
    if (usec && usec->is_integer()) {
        result->set_subsec(env, usec->as_integer());
    }
    return result;
}

TimeObject *TimeObject::create(Env *env) {
    return now(env, nullptr);
}

TimeObject *TimeObject::initialize(Env *env, Value year, Value month, Value mday, Value hour, Value min, Value sec, Value zone) {
    if (!year) {
        return now(env, nullptr);
    } else if (year->is_nil()) {
        env->raise("TypeError", "Year cannot be nil");
    } else {

        auto result = now(env, nullptr);
        result->build_time(env, year, month, mday, hour, min, sec);
        int seconds = mktime(&result->m_time);
        result->m_mode = Mode::Localtime;
        result->m_integer = Value::integer(seconds);
        result->m_subsec = nullptr;
        return result;
    }
}

TimeObject *TimeObject::now(Env *env, Value _in) {
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
    TimeObject *result = new TimeObject {};
    result->build_time(env, year, month, mday, hour, min, sec);
    result->m_time.tm_gmtoff = 0;
    int seconds = timegm(&result->m_time);
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
        ::strftime(buffer, 7, " %z", &m_time);
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

Value TimeObject::minus(Env *env, Value other) {
    if (other->is_time()) {
        return to_r(env)->as_rational()->sub(env, other->as_time()->to_r(env))->as_rational()->to_f(env);
    }
    if (other->is_nil() || !other->respond_to(env, "to_r"_s)) {
        env->raise("TypeError", "can't convert {} into an exact number", other->klass()->inspect_str());
    }
    RationalObject *rational = to_r(env)->as_rational()->sub(env, other->send(env, "to_r"_s))->as_rational();
    return TimeObject::create(env, rational, m_mode);
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

Value TimeObject::to_a(Env *env) const {
    Value dstval = isdst(env) ? (Value)(TrueObject::the()) : (Value)(FalseObject::the());
    return new ArrayObject { sec(env), min(env), hour(env), mday(env), month(env), year(env), wday(env), yday(env), dstval, zone(env) };
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

Value TimeObject::utc_offset(Env *env) const {
    return IntegerObject::create((nat_int_t)m_time.tm_gmtoff);
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

nat_int_t TimeObject::normalize_field(Env *env, Value val) {
    if (!val->is_integer()) {
        if (val->respond_to(env, "to_i"_s)) {
            val = val->send(env, "to_i"_s);
        } else if (val->respond_to(env, "to_int"_s)) {
            val = val->send(env, "to_int"_s);
        }
    }
    val->assert_type(env, Object::Type::Integer, "Integer");
    return val->as_integer()->to_nat_int_t();
}

nat_int_t TimeObject::normalize_field(Env *env, Value val, nat_int_t minval, nat_int_t maxval) {
    if (val->is_nil()) return minval;
    auto ival = normalize_field(env, val);
    if (ival < minval || ival > maxval) {
        env->raise("ArgumentError", "argument out of range");
    }
    return ival;
}

nat_int_t TimeObject::normalize_month(Env *env, Value val) {
    if (val->is_nil()) return 0;
    if (!val->is_integer()) {
        if (!val->is_string() && val->respond_to(env, "to_str"_s)) {
            val = val->send(env, "to_str"_s)->as_string();
        }
        if (val->is_string()) {
            auto monstr = val->as_string()->downcase(env, nullptr, nullptr)->as_string()->string();
            if (monstr == "jan") {
                return 0;
            } else if (monstr == "feb") {
                return 1;
            } else if (monstr == "mar") {
                return 2;
            } else if (monstr == "apr") {
                return 3;
            } else if (monstr == "may") {
                return 4;
            } else if (monstr == "jun") {
                return 5;
            } else if (monstr == "jul") {
                return 6;
            } else if (monstr == "aug") {
                return 7;
            } else if (monstr == "sep") {
                return 8;
            } else if (monstr == "oct") {
                return 9;
            } else if (monstr == "nov") {
                return 10;
            } else if (monstr == "dec") {
                return 11;
            }
            auto monint = KernelModule::Integer(env, val, 10, true)->as_integer()->to_nat_int_t();
            if (monint >= 1 && monint <= 12) {
                return monint - 1;
            }
            env->raise("ArgumentError", "mon out of range");
        }
        if (val->respond_to(env, "to_int"_s)) {
            val = val->send(env, "to_int"_s);
        }
    }
    val->assert_type(env, Object::Type::Integer, "Integer");
    auto month_i = val->as_integer()->to_nat_int_t() - 1;
    if (month_i < 0 || month_i > 11) {
        env->raise("ArgumentError", "mon out of range");
    }
    return month_i;
}

RationalObject *TimeObject::convert_rational(Env *env, Value value) {
    if (value->is_integer()) {
        return RationalObject::create(env, value->as_integer(), new IntegerObject { 1 });
    } else if (value->is_rational()) {
        return value->as_rational();
    } else if (value->respond_to(env, "to_r"_s) && value->respond_to(env, "to_int"_s)) {
        return value->send(env, "to_r"_s)->as_rational();
    } else {
        env->raise("TypeError", "can't convert {} into an exact number", value->klass()->inspect_str());
    }
}

Value TimeObject::convert_unit(Env *env, Value value) {
    if (value == "millisecond"_s) {
        return Value::integer(1000);
    } else if (value == "microsecond"_s || value == "usec"_s) {
        return Value::integer(1000000);
    } else if (value == "nanosecond"_s || value == "nsec"_s) {
        return Value::integer(1000000000);
    } else {
        env->raise("ArgumentError", "unexpected unit: {}", value->inspect_str(env));
    }
}

TimeObject *TimeObject::create(Env *env, RationalObject *rational, Mode mode) {
    IntegerObject *integer;
    RationalObject *subseconds;
    TimeObject *result = new TimeObject {};
    if (rational->send(env, "<"_s, { Value::integer(0) })->is_true()) {
        auto floor = rational->floor(env, nullptr);
        integer = floor->send(env, "to_i"_s)->as_integer();
        subseconds = rational->sub(env, floor)->as_rational();
    } else {
        integer = rational->to_i(env)->as_integer();
        subseconds = rational->sub(env, integer)->as_rational();
    }
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

void TimeObject::build_time(Env *env, Value year, Value month, Value mday, Value hour, Value min, Value sec) {
    m_time = { 0 };
    m_time.tm_year = TimeObject::normalize_field(env, year) - 1900;
    m_time.tm_mon = 0;
    m_time.tm_mday = 1;
    m_time.tm_hour = 0;
    m_time.tm_min = 0;
    m_time.tm_sec = 0;
    m_time.tm_isdst = -1; // dst-unknown == -1 ; mktime() will set to 0 or 1.

    if (month) {
        m_time.tm_mon = TimeObject::normalize_month(env, month);
    }
    if (mday) {
        m_time.tm_mday = TimeObject::normalize_field(env, mday, 1, 31);
    }
    if (hour) {
        m_time.tm_hour = TimeObject::normalize_field(env, hour, 0, 24);
    }
    if (min) {
        m_time.tm_min = TimeObject::normalize_field(env, min, 0, 59);
    }
    if (sec && !sec->is_nil()) {
        if (sec->is_integer()) {
            auto sec_i = sec->as_integer()->to_nat_int_t();
            if (sec_i < 0 || sec_i > 59) {
                env->raise("ArgumentError", "argument out of range");
            }
            m_time.tm_sec = sec_i;
        } else {
            RationalObject *rational = convert_rational(env, sec);
            auto divmod = rational->send(env, "divmod"_s, { Value::integer(1) })->as_array();
            m_time.tm_sec = divmod->first()->as_integer()->to_nat_int_t();
            set_subsec(env, divmod->last()->as_rational());
        }
    }
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
    return new StringObject { buffer, length, EncodingObject::get(Encoding::US_ASCII) };
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

// NATFIXME: support writing actual zone string/object
// based on m_zone
Value TimeObject::zone(Env *env) const {
    if (is_utc(env)) {
        return new StringObject { "UTC", Encoding::US_ASCII };
    }
    return NilObject::the();
    // return new StringObject { "", Encoding::US_ASCII };
}

}
