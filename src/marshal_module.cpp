#include "natalie.hpp"

extern "C" char *dtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve);

namespace Natalie {

Value MarshalModule::float_to_string(Env *env, Value value) {
    int decpt, sign;
    char *out, *e;
    out = dtoa(value.as_float()->to_double(), 0, 0, &decpt, &sign, &e);
    int digits = strlen(out);
    String string;
    if (decpt < -3 || decpt > digits) {
        string.append(out);
        string.insert(1, '.');
        string.append_sprintf("e%d", decpt - 1);
    } else if (decpt > 0) {
        string.append(out);
        if (decpt < digits) {
            string.insert(decpt, '.');
        }
    } else {
        string = String("0.");
        string.append(::abs(decpt), '0');
        string.append(out);
    }
    if (sign) {
        string.prepend_char('-');
    }
    return StringObject::create(string, Encoding::US_ASCII);
}

void MarshalModule::set_exception_cause(Env *env, Value exception, Value cause) {
    exception.as_exception_or_raise(env)->set_cause(cause.as_exception_or_raise(env));
}

}
