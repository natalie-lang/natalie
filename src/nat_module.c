#include "natalie.h"
#include "nat_class.h"
#include "nat_module.h"
#include "nat_object.h"

NatObject *Module_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_module(env, NULL);
}

NatObject *Module_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_MODULE);
    if (self->class_name) {
        return nat_string(env, self->class_name);
    } else {
        return Object_inspect(env, self, argc, args, kwargs);
    }
}
