#include "natalie.h"
#include "nat_class.h"

NatObject *Class_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    NatObject *Class = env_get(env, "Class");
    return nat_subclass(Class, "<FIXME>");
}

NatObject *Class_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_CLASS);
    return nat_string(env, self->class_name);
}

NatObject *Class_include(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_CLASS);
    assert(argc >= 1);
    for (size_t i=0; i<argc; i++) {
        nat_class_include(self, args[i]);
    }
    return self;
}

NatObject *Class_included_modules(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_CLASS);
    NatObject *modules = nat_array(env);
    for (size_t i=0; i<self->included_modules_count; i++) {
        nat_array_push(modules, self->included_modules[i]);
    }
    return modules;
}

