#include "natalie.hpp"
#include "natalie/integer_methods.hpp"

namespace Natalie {

TimeObject *TimeObject::at(Env *env, ClassObject *klass, Value time, Optional<Value> subsec, Optional<Value> unit) {
    if (time.is_a(env, GlobalEnv::the()->Time()))
        return KernelModule::dup(env, time).as_time();
    RationalObject *rational = convert_rational(env, time);
    if (subsec) {
        auto scale = convert_unit(env, unit.value_or("microsecond"_s));
        rational = rational->add(env, convert_rational(env, subsec.value())->div(env, scale)).as_rational();
    }
    return create(env, klass, rational, Mode::Localtime);
}

TimeObject *TimeObject::at(Env *env, ClassObject *klass, Value time, Optional<Value> subsec, Optional<Value> unit, Optional<Value> in) {
    if (time.is_a(env, GlobalEnv::the()->Time()) && subsec)
        env->raise("TypeError", "can't convert {} into an exact number", time->klass()->inspected(env));
    auto result = at(env, klass, time, subsec, unit);
    if (in) {
        static const auto utc_to_local = "utc_to_local"_s;
        result->m_time.tm_gmtoff = normalize_timezone(env, in.value(), utc_to_local);
        result->m_zone = strdup("UTC");
        result->m_time.tm_zone = result->m_zone;
    }
    return result;
}

TimeObject *TimeObject::local(Env *env, Value year, Optional<Value> month, Optional<Value> mday, Optional<Value> hour, Optional<Value> min, Optional<Value> sec, Optional<Value> usec) {
    TimeObject *result = TimeObject::create();
    result->build_time(env, year, month, mday, hour, min, sec);
    int seconds = mktime(&result->m_time);
    result->m_mode = Mode::Localtime;
    result->m_integer = seconds;
    if (usec && usec->is_integer()) {
        result->set_subsec(env, usec->integer());
    }
    return result;
}

TimeObject *TimeObject::create(Env *env) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    return now(env, nullptr);
}

// Time.new
TimeObject *TimeObject::initialize(Env *env, Optional<Value> year, Optional<Value> month, Optional<Value> mday, Optional<Value> hour, Optional<Value> min, Optional<Value> sec, Optional<Value> tmzone, Optional<Value> in) {
    if (!year)
        return now(env, nullptr);

    if (year->is_nil()) {
        env->raise("TypeError", "Year cannot be nil");
    } else {
        auto result = now(env, nullptr);
        result->build_time(env, year.value(), month, mday, hour, min, sec);
        int seconds = mktime(&result->m_time);
        result->m_integer = seconds;
        result->m_subsec = Optional<Value>();
        static const auto local_to_utc = "local_to_utc"_s;
        if (tmzone && in && !tmzone->is_nil() && !in->is_nil()) {
            env->raise("ArgumentError", "timezone argument given as positional and keyword arguments");
        } else if (tmzone && !tmzone->is_nil()) {
            result->m_time.tm_gmtoff = normalize_timezone(env, tmzone.value(), local_to_utc);
            result->m_mode = Mode::UTC;
        } else if (in && !in->is_nil()) {
            result->m_time.tm_gmtoff = normalize_timezone(env, in.value(), local_to_utc);
            result->m_mode = Mode::UTC;
        } else {
            result->m_mode = Mode::Localtime;
        }

        return result;
    }
}

TimeObject *TimeObject::now(Env *env, ClassObject *klass, Optional<Value> in) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    struct tm time = *localtime(&ts.tv_sec);
    TimeObject *result;
    if (klass)
        result = TimeObject::create(klass);
    else
        result = TimeObject::create();
    result->m_time = time;
    result->m_mode = Mode::Localtime;
    result->m_integer = ts.tv_sec;
    result->set_subsec(env, ts.tv_nsec);
    if (in) {
        static const auto utc_to_local = "utc_to_local"_s;
        result->m_time.tm_gmtoff = normalize_timezone(env, in.value(), utc_to_local);
    }
    return result;
}

TimeObject *TimeObject::utc(Env *env, Value year, Optional<Value> month, Optional<Value> mday, Optional<Value> hour, Optional<Value> min, Optional<Value> sec, Optional<Value> subsec_arg) {
    TimeObject *result = TimeObject::create();
    result->build_time(env, year, month, mday, hour, min, sec);
    result->m_time.tm_gmtoff = 0;
    int seconds = timegm(&result->m_time);
    result->m_mode = Mode::UTC;
    result->m_integer = seconds;
    if (subsec_arg) {
        auto subsec = subsec_arg.value();
        if (subsec.is_integer()) {
            auto integer = subsec.integer();
            if (integer < 0 || integer >= 1000000)
                env->raise("ArgumentError", "subsecx out of range");
            result->m_subsec = RationalObject::create(env, integer, Integer(1000000));
        } else if (subsec.is_rational()) {
            result->m_subsec = subsec.as_rational()->div(env, Value::integer(1000000));
        } else {
            env->raise("TypeError", "can't convert {} into an exact number", subsec.klass()->inspect_module());
        }
    }
    return result;
}

Value TimeObject::add(Env *env, Value other) {
    if (other.is_time()) {
        env->raise("TypeError", "time + time?");
    } else if (other.is_nil() || other.is_string() || !other.respond_to(env, "to_r"_s)) {
        env->raise("TypeError", "can't convert {} into an exact number", other.klass()->inspect_module());
    }
    RationalObject *rational = to_r(env).as_rational();
    rational = rational->add(env, other.send(env, "to_r"_s).as_rational()).as_rational();
    auto result = TimeObject::create(env, GlobalEnv::the()->Time(), rational, m_mode);
    if (is_utc(env)) { // preserve utc-offset for result if a utc-time
        result->m_time.tm_gmtoff = utc_offset(env).integer().to_nat_int_t();
    }
    return result;
}

Value TimeObject::asctime(Env *env) {
    return build_string(&m_time, "%a %b %e %T %Y");
}

Value TimeObject::cmp(Env *env, Value other) {
    if (other.is_time()) {
        auto time = other.as_time();
        if (m_integer > time->m_integer) {
            return Value::integer(1);
        } else if (m_integer < time->m_integer) {
            return Value::integer(-1);
        } else {
            return subsec(env).send(env, "to_r"_s).as_rational()->cmp(env, time->subsec(env).send(env, "to_r"_s));
        }
    } else {
        auto result = other.send(env, "<=>"_s, { this });
        if (result.is_nil()) {
            return result;
        } else {
            if (result.send(env, ">"_s, { Value::integer(0) }).is_true()) {
                return Value::integer(-1);
            } else if (result.send(env, "<"_s, { Value::integer(0) }).is_true()) {
                return Value::integer(1);
            } else {
                return Value::integer(0);
            }
        }
    }
}

bool TimeObject::eql(Env *env, Value other) {
    if (other.is_time()) {
        auto time = other.as_time();
        if (m_integer == time->m_integer) {
            if (m_subsec && time->m_subsec && m_subsec->as_rational()->eq(env, time->m_subsec.value())) {
                return true;
            } else if (!m_subsec && !time->m_subsec) {
                return true;
            }
        }
    }
    return false;
}

Value TimeObject::gmtime(Env *env) {
    if (is_utc(env))
        return this;
    if (is_frozen())
        env->raise("FrozenError", "blaap");
    auto next_obj = to_utc(env).as_time();
    m_integer = next_obj->m_integer;
    m_mode = next_obj->m_mode;
    m_subsec = next_obj->m_subsec;
    m_time = next_obj->m_time;
    free(m_zone);
    m_zone = strdup("UTC");
    return this;
}

Value TimeObject::hour(Env *) const {
    return Value::integer(m_time.tm_hour);
}

Value TimeObject::inspect(Env *env) {
    StringObject *result = build_string(&m_time, "%Y-%m-%d %H:%M:%S").as_string();
    if (m_subsec) {
        auto integer = m_subsec->as_rational()->mul(env, Value::integer(1000000000)).as_rational()->to_i(env);
        auto string = integer.to_s(env);
        auto length = string->length();
        if (length > 9) {
            string->truncate(9);
        } else if (length < 9) {
            TM::String string_with_prefix(9 - length, '0');
            string_with_prefix.append(string->string());
            string->set_str(std::move(string_with_prefix));
        }
        result->append_char('.');
        result->append(strip_zeroes(string));
    }
    if (m_time.tm_gmtoff) {
        char buffer[7];
        ::strftime(buffer, 7, " %z", &m_time);
        result->append(buffer);
    } else {
        result->append(" UTC");
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
    if (other.is_time()) {
        return to_r(env).as_rational()->sub(env, other.as_time()->to_r(env)).as_rational()->to_f(env);
    }
    if (other.is_nil() || other.is_string() || !other.respond_to(env, "to_r"_s)) {
        env->raise("TypeError", "can't convert {} into an exact number", other.klass()->inspect_module());
    }
    RationalObject *rational = to_r(env).as_rational()->sub(env, other.send(env, "to_r"_s)).as_rational();
    auto result = TimeObject::create(env, GlobalEnv::the()->Time(), rational, m_mode);
    if (is_utc(env)) { // preserve utc-offset for result if a utc-time
        result->m_time.tm_gmtoff = utc_offset(env).integer().to_nat_int_t();
    }
    return result;
}

Value TimeObject::month(Env *) const {
    return Value::integer(m_time.tm_mon + 1);
}

Value TimeObject::nsec(Env *env) {
    if (m_subsec) {
        return m_subsec->as_rational()->mul(env, Value::integer(1000000000)).as_rational()->to_i(env);
    } else {
        return Value::integer(0);
    }
}

Value TimeObject::sec(Env *) const {
    return Value::integer(m_time.tm_sec);
}

Value TimeObject::strftime(Env *env, Value format) {
    return build_string(&m_time, format.as_string()->c_str(), format.as_string()->encoding()->num());
}

Value TimeObject::subsec(Env *) {
    if (m_subsec) {
        return m_subsec.value();
    } else {
        return Value::integer(0);
    }
}

Value TimeObject::to_a(Env *env) const {
    auto dstval = bool_object(isdst(env));
    return ArrayObject::create({ sec(env), min(env), hour(env), mday(env), month(env), year(env), wday(env), yday(env), dstval, zone(env) });
}

Value TimeObject::to_f(Env *env) {
    Value result = IntegerMethods::to_f(m_integer);
    if (m_subsec) {
        result = result.as_float()->add(env, m_subsec->as_rational());
    }
    return result;
}

Value TimeObject::to_r(Env *env) {
    Value result = RationalObject::create(env, m_integer, Integer(1));
    if (m_subsec) {
        result = result.as_rational()->add(env, m_subsec->as_rational());
    }
    return result;
}

Value TimeObject::to_s(Env *env) {
    if (m_time.tm_gmtoff) {
        return build_string(&m_time, "%Y-%m-%d %H:%M:%S %z");
    } else {
        return build_string(&m_time, "%Y-%m-%d %H:%M:%S UTC");
    }
}

Value TimeObject::to_utc(Env *env) {
    return utc(env, year(env), month(env), mday(env), hour(env), min(env), sec(env), subsec(env))->minus(env, utc_offset(env));
}

Value TimeObject::usec(Env *env) {
    if (m_subsec) {
        return m_subsec->as_rational()->mul(env, Value::integer(1000000)).as_rational()->to_i(env);
    } else {
        return Value::integer(0);
    }
}

Value TimeObject::utc_offset(Env *env) const {
    return Value::integer((nat_int_t)m_time.tm_gmtoff);
}

Value TimeObject::wday(Env *) const {
    return Value::integer(m_time.tm_wday);
}

Value TimeObject::xmlschema(Env *env, Optional<Value> fraction_digits) const {
    auto result = build_string(&m_time, "%04Y-%m-%dT%H:%M:%S").as_string();
    if (fraction_digits) {
        auto digits = IntegerMethods::convert_to_native_type<size_t>(env, fraction_digits->to_int(env));
        if (digits > 0) {
            StringObject *subsec;
            if (m_subsec) {
                subsec = m_subsec->as_rational()->mul(env, Value::integer(1000000000)).as_rational()->to_i(env).to_s(env);
            } else {
                subsec = StringObject::create("0");
            };
            auto length = subsec->length();
            if (length > digits) {
                subsec->truncate(digits);
            } else if (length < digits) {
                subsec = subsec->send(env, "ljust"_s, { Value::integer(digits), StringObject::create("0") }).as_string();
            }
            result->append_char('.');
            result->append(subsec);
        }
    }
    if (m_time.tm_gmtoff) {
        char buffer[7];
        ::strftime(buffer, 7, "%z", &m_time);
        memmove(buffer + 4, buffer + 3, 3);
        buffer[3] = ':';
        result->append(buffer);
    } else {
        result->append_char('Z');
    }
    return result;
}

Value TimeObject::yday(Env *) const {
    return Value::integer(m_time.tm_yday + 1);
}

Value TimeObject::year(Env *) const {
    return Value::integer(m_time.tm_year + 1900);
}

// utc-offset and military timezone decoding
nat_int_t TimeObject::normalize_timezone(Env *env, Value val, NonNullPtr<SymbolObject> tzobj) {
    nat_int_t minsec = 60; // seconds in an minute
    nat_int_t hoursec = 3600; // seconds in an hour

    static const auto to_str = "to_str"_s;
    if (val.is_string() || val.respond_to(env, to_str)) {
        auto str = val.to_str(env)->string();
        auto ssize = str.size();
        if (str == "UTC") {
            return 0;
        } else if (ssize == 1) {
            char mil_tz = str.at(0);
            // https://en.wikipedia.org/wiki/List_of_military_time_zones
            if (mil_tz >= 'A' && mil_tz <= 'I') {
                return (mil_tz - 'A' + 1) * hoursec;
            } else if (mil_tz >= 'K' && mil_tz <= 'M') {
                return (mil_tz - 'K' + 10) * hoursec;
            } else if (mil_tz >= 'N' && mil_tz <= 'Y') {
                return (mil_tz - 'N' + 1) * -1 * hoursec;
            } else if (mil_tz == 'Z') {
                return 0;
            }
            // other single characters are illegal
        } else if (ssize == 6 || ssize == 9) {
            char sign = str.at(0);
            if ((sign == '+' || sign == '-') && isdigit(str[1]) && isdigit(str[2]) && str[3] == ':' && isdigit(str[4]) && isdigit(str[5]) && (ssize == 6 || (str[6] == ':' && isdigit(str[7]) && isdigit(str[8])))) {

                nat_int_t isign = (sign == '+') ? 1 : -1;
                auto hour = atoi(str.substring(1, 2).c_str());
                auto min = atoi(str.substring(4, 2).c_str());
                nat_int_t sec = 0;
                if (ssize == 9) {
                    sec = atoi(str.substring(7, 2).c_str());
                }
                if (hour < 24 && min < 60 && sec < 60) {
                    return isign * ((hour * hoursec) + (min * minsec) + sec);
                } else {
                    env->raise("ArgumentError", "utc_offset out of range");
                }
            }
        }
        // any fall-through of the above ugly logic
        env->raise("ArgumentError", "\"+HH:MM\", \"-HH:MM\", \"UTC\" or \"A\"..\"I\",\"K\"..\"Z\" expected for utc_offset: {}", str);
    }
    if (tzobj && val.respond_to(env, tzobj.ptr())) {
        val = val.send(env, tzobj.ptr(), { Value::integer(0) });
        static const auto local_to_utc = "local_to_utc"_s;
        if (tzobj == local_to_utc)
            val = IntegerMethods::sub(env, 0, val);
    }
    static const auto to_int = "to_int"_s;
    if (val.is_integer() || val.respond_to(env, to_int)) {
        auto seconds = val.to_int(env).to_nat_int_t();
        if (seconds > hoursec * -24 && seconds < hoursec * 24) {
            return seconds;
        }
        env->raise("ArgumentError", "utc_offset out of range");
    }
    env->raise("TypeError", "can't convert {} into an exact number", val.klass()->inspected(env));
}

nat_int_t TimeObject::normalize_field(Env *env, Value val) {
    if (!val.is_integer() && val.respond_to(env, "to_i"_s))
        val = val.send(env, "to_i"_s);
    return val.to_int(env).to_nat_int_t();
}

nat_int_t TimeObject::normalize_field(Env *env, Value val, nat_int_t minval, nat_int_t maxval) {
    if (val.is_nil()) return minval;
    auto ival = normalize_field(env, val);
    if (ival < minval || ival > maxval) {
        env->raise("ArgumentError", "argument out of range");
    }
    return ival;
}

nat_int_t TimeObject::normalize_month(Env *env, Value val) {
    if (val.is_nil()) return 0;
    if (!val.is_integer()) {
        if (val.is_string() || val.respond_to(env, "to_str"_s)) {
            val = val.to_str(env);
            auto monstr = val.as_string()->downcase(env)->string();
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
            auto monint = KernelModule::Integer(env, val, 10, true).integer().to_nat_int_t();
            if (monint >= 1 && monint <= 12) {
                return monint - 1;
            }
            env->raise("ArgumentError", "mon out of range");
        }
        if (val.respond_to(env, "to_int"_s)) {
            val = val.to_int(env);
        }
    }
    val.assert_integer(env);
    auto month_i = val.integer() - 1;
    if (month_i < 0 || month_i > 11) {
        env->raise("ArgumentError", "mon out of range");
    }
    return month_i.to_nat_int_t();
}

RationalObject *TimeObject::convert_rational(Env *env, Value value) {
    if (value.is_integer()) {
        return RationalObject::create(env, value.integer(), Integer(1));
    } else if (value.is_rational()) {
        return value.as_rational();
    } else if (value.respond_to(env, "to_r"_s) && value.respond_to(env, "to_int"_s)) {
        return value.send(env, "to_r"_s).as_rational();
    } else if (value.respond_to(env, "to_int"_s)) {
        return RationalObject::create(env, value.to_int(env), Integer(1));
    } else {
        env->raise("TypeError", "can't convert {} into an exact number", value.klass()->inspect_module());
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
        env->raise("ArgumentError", "unexpected unit: {}", value.inspected(env));
    }
}

TimeObject *TimeObject::create(Env *env, ClassObject *klass, RationalObject *rational, Mode mode) {
    Integer integer;
    RationalObject *subseconds;
    TimeObject *result;
    if (klass)
        result = TimeObject::create(klass);
    else
        result = TimeObject::create();
    if (rational->send(env, "<"_s, { Value::integer(0) }).is_true()) {
        auto floor = rational->floor(env);
        integer = floor.send(env, "to_i"_s).integer();
        subseconds = rational->sub(env, floor).as_rational();
    } else {
        integer = rational->to_i(env).integer();
        subseconds = rational->sub(env, integer).as_rational();
    }
    time_t seconds = (time_t)integer.to_nat_int_t();
    if (mode == Mode::UTC) {
        result->m_time = *::gmtime(&seconds);
    } else {
        result->m_time = *localtime(&seconds);
    }
    result->m_mode = mode;
    result->m_integer = integer;
    result->set_subsec(env, subseconds);
    return result;
}

void TimeObject::build_time(Env *env, Value year, Optional<Value> month, Optional<Value> mday, Optional<Value> hour, Optional<Value> min, Optional<Value> sec_arg) {
    m_time = { 0 };
    m_time.tm_year = TimeObject::normalize_field(env, year) - 1900;
    m_time.tm_mon = 0;
    m_time.tm_mday = 1;
    m_time.tm_hour = 0;
    m_time.tm_min = 0;
    m_time.tm_sec = 0;
    m_time.tm_isdst = -1; // dst-unknown == -1 ; mktime() will set to 0 or 1.

    if (month) {
        m_time.tm_mon = TimeObject::normalize_month(env, month.value());
    }
    if (mday) {
        m_time.tm_mday = TimeObject::normalize_field(env, mday.value(), 1, 31);
    }
    if (hour) {
        m_time.tm_hour = TimeObject::normalize_field(env, hour.value(), 0, 24);
    }
    if (min) {
        m_time.tm_min = TimeObject::normalize_field(env, min.value(), 0, 59);
    }
    if (sec_arg && !sec_arg->is_nil()) {
        auto sec = sec_arg.value();
        if (sec.is_string()) {
            // ensure base10 conversion for case of "01" input
            sec = KernelModule::Integer(env, sec, 10, true);
        }
        if (sec.is_integer()) {
            auto sec_i = sec.integer().to_nat_int_t();
            if (sec_i < 0 || sec_i > 59) {
                env->raise("ArgumentError", "argument out of range");
            }
            m_time.tm_sec = sec_i;
        } else {
            RationalObject *rational = convert_rational(env, sec);
            auto divmod = rational->send(env, "divmod"_s, { Value::integer(1) }).as_array();
            m_time.tm_sec = divmod->first().integer().to_nat_int_t();
            set_subsec(env, divmod->last().as_rational());
        }
    }
}

void TimeObject::set_subsec(Env *env, long nsec) {
    if (nsec > 0) {
        m_subsec = RationalObject::create(env, Integer(nsec), Integer(1000000000));
    }
}

void TimeObject::set_subsec(Env *env, Integer usec) {
    if (usec < 0 || usec >= 1000000)
        env->raise("ArgumentError", "subsecx out of range");

    if (!usec.is_zero())
        m_subsec = RationalObject::create(env, usec, Integer(1000000));
}

void TimeObject::set_subsec(Env *, RationalObject *subsec) {
    if (!subsec->is_zero())
        m_subsec = subsec;
}

Value TimeObject::build_string(const tm *time, const char *format, Encoding encoding) {
    int maxsize = 32;
    auto buffer = new char[maxsize];
    auto length = ::strftime(buffer, maxsize, format, time);
    String str { std::move(buffer), length };
    return StringObject::create(std::move(str), encoding);
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
        return StringObject::create();
    } else {
        size_t new_length = static_cast<size_t>(last_char + 1);
        return StringObject::create(c_str, new_length);
    }
}

// NATFIXME: unclear conditions to return nil, logic may be off here.
Value TimeObject::zone(Env *env) const {
    if (m_time.tm_gmtoff != 0 || !m_zone) {
        return Value::nil();
    }
    if (is_utc(env)) {
        return StringObject::create("UTC", Encoding::US_ASCII);
    }
    return StringObject::create(m_zone, Encoding::US_ASCII);
}

}
