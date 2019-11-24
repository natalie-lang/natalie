#include "natalie.h"
#include "nat_object.h"

NatObject *Object_puts(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    NatObject *str;
    for (size_t i=0; i<argc; i++) {
        str = nat_send(env, args[i], "to_s", 0, NULL);
        assert(str->type == NAT_VALUE_STRING);
        printf("%s\n", str->str);
    }
    return env_get(env, "nil");
}

NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_new(self);
}

NatObject *Object_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    return nat_string(env, "<Object FIXME>");
}
