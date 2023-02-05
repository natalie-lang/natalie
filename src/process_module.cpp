#include "natalie.hpp"

namespace Natalie {

Value ProcessModule::groups(Env *env) {
    const auto size = getgroups(0, nullptr);
    gid_t list[size];
    if (getgroups(size, list) < 0)
        env->raise_errno();
    auto result = new ArrayObject { static_cast<size_t>(size) };
    // MacOS does not include the effective GID in the output of getgroups
    const auto egid = getegid();
    bool has_egid = false;
    for (int i = 0; i < size; i++) {
        result->push(Value::integer(list[i]));
        has_egid = has_egid || list[i] == egid;
    }
    if (!has_egid)
        result->push(Value::integer(egid));
    return result;
}

}
