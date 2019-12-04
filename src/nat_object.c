#include "natalie.h"
#include "nat_object.h"
#include "nat_class.h"

NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_new(env, self);
}

NatObject *Object_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    NatObject *str = nat_string(env, "#<");
    assert(self->class);
    nat_string_append(str, Class_inspect(env, self->class, 0, NULL, NULL)->str);
    nat_string_append_char(str, ':');
    nat_string_append(str, nat_object_pointer_id(self));
    nat_string_append_char(str, '>');
    return str;
}

NatObject *Object_object_id(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_integer(env, self->id);
}

NatObject *Object_equal(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(argc == 1);
    NatObject *arg = args[0];
    if (self == arg) {
        return env_get(env, "true");
    } else {
        return env_get(env, "false");
    }
}

NatObject *Object_class(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return self->class ? self->class : env_get(env, "nil");
}
