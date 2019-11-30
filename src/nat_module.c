#include "natalie.h"
#include "nat_module.h"
#include "nat_class.h"

NatObject *Module_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_module(env, NULL);
}

NatObject *Module_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_MODULE);
    if (self->class_name) {
        return nat_string(env, self->class_name);
    } else {
        NatObject *str = nat_string(env, "#<");
        assert(self->class);
        nat_string_append(str, Class_inspect(env, self->class, 0, NULL, NULL)->str);
        nat_string_append_char(str, ':');
        nat_string_append(str, nat_object_id(self));
        nat_string_append_char(str, '>');
        return str;
    }
}
