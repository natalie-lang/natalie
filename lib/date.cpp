#include "natalie.hpp"

using namespace Natalie;

Value init_date(Env *env, Value self) { return Value::nil(); }

Value Date_strftime(Env *env, Value self, Value format) {
    format.assert_type(env, Object::Type::String, "String");
    struct tm time = { 0 };
    time.tm_year = IntegerMethods::convert_to_int(env, self->ivar_get(env, "@year"_s)) - 1900;
    time.tm_mon = IntegerMethods::convert_to_int(env, self->ivar_get(env, "@month"_s)) - 1;
    time.tm_mday = IntegerMethods::convert_to_int(env, self->ivar_get(env, "@mday"_s));
    time.tm_gmtoff = 0;
    time.tm_isdst = 0;
    return TimeObject::build_string(&time, format.as_string()->c_str(), format.as_string()->encoding()->num());
}
