#include "builtin.h"
#include "natalie.h"

NatObject *Process_pid(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    pid_t pid = getpid();
    return nat_integer(env, pid);
}
