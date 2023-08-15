#include "natalie.hpp"

#include <time.h>

namespace Natalie {

Value ProcessModule::clock_gettime(Env *env, Value clock_id) {
    timespec tp;
    const auto clk_id = static_cast<clockid_t>(IntegerObject::convert_to_nat_int_t(env, clock_id));
    if (::clock_gettime(clk_id, &tp) < 0)
        env->raise_errno();

    double result = static_cast<double>(tp.tv_sec) + tp.tv_nsec / static_cast<double>(1000000000);
    return new FloatObject { result };
}

}
