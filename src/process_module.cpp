#include "natalie.hpp"

#include <time.h>

namespace Natalie {

Value ProcessModule::groups(Env *env) {
    auto size = getgroups(0, nullptr);
    if (size < 0)
        env->raise_errno();
    gid_t list[size];
    size = getgroups(size, list);
    if (size < 0)
        env->raise_errno();
    auto result = new ArrayObject { static_cast<size_t>(size) };
    for (size_t i = 0; i < static_cast<size_t>(size); i++) {
        result->push(IntegerObject::create(list[i]));
    }
    return result;
}

Value ProcessModule::clock_gettime(Env *env, Value clock_id) {
    timespec tp;
    const auto clk_id = static_cast<clockid_t>(IntegerObject::convert_to_nat_int_t(env, clock_id));
    if (::clock_gettime(clk_id, &tp) < 0)
        env->raise_errno();

    double result = static_cast<double>(tp.tv_sec) + tp.tv_nsec / static_cast<double>(1000000000);
    return new FloatObject { result };
}

}
