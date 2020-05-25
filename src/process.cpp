#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Process_pid(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    pid_t pid = getpid();
    return integer(env, pid);
}

}
