#include "natalie.h"
#include "nat_module.h"

// TODO
//NatObject *Module_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
//    NatObject *Class = env_get(env, "Class");
//    return nat_subclass(Class, "<FIXME>");
//}

NatObject *Module_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_MODULE);
    return nat_string(env, self->class_name);
}
