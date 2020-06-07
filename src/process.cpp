#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Process_pid(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    pid_t pid = getpid();
    return integer(env, pid);
}

}
