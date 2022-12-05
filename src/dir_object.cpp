#include "natalie.hpp"

#include <errno.h>
#include <sys/stat.h>

namespace Natalie {

Value DirObject::mkdir(Env *env, Value path, Value mode) {
    mode_t octmode = 0777;
    if (mode)
        octmode = (mode_t)(mode->as_integer()->to_nat_int_t());
    path->assert_type(env, Object::Type::String, "String");
    auto result = ::mkdir(path->as_string()->c_str(), octmode);
    if (result < 0) env->raise_errno();
    // need to check dir exists and return nil if mkdir was unsuccessful.
    return Value::integer(0);
}

Value DirObject::rmdir(Env *env, Value path) {
    path->assert_type(env, Object::Type::String, "String");
    auto result = ::rmdir(path->as_string()->c_str());
    if (result < 0) env->raise_errno();
    return Value::integer(0);
}

}
