#include "natalie.h"
#include "nat_class.h"

NatObject *Class_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    NatObject *Class = env_get(env, "Class");
    return nat_subclass(Class, "<FIXME>");
}
