#include "natalie.h"
#include "nat_object.h"
#include "nat_class.h"

NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_new(self);
}

NatObject *Object_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    NatObject *str = nat_string(env, "#<");
    assert(self->class);
    nat_string_append(str, Class_inspect(env, self->class, 0, NULL, NULL)->str);
    nat_string_append_char(str, ':');
    nat_string_append(str, nat_object_id(self));
    nat_string_append_char(str, '>');
    return str;
}
