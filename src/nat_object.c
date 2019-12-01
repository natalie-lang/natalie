#include "natalie.h"
#include "nat_object.h"

NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_new(self);
}

NatObject *Object_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_string(env, "<Object FIXME>");
}
