#include "natalie.h"
#include "nat_exception.h"
#include "nat_class.h"
#include "nat_object.h"

NatObject *Exception_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0, 1);
    NatObject *exception = Object_new(env, self, argc, args, kwargs, block);
    exception->type = NAT_VALUE_EXCEPTION;
    if (exception->message == NULL) exception->message = self->class_name;
    return exception;
}

NatObject *Exception_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    if (argc == 0) {
        self->message = self->class_name;
    } else if (argc == 1) {
        NatObject *message = args[0];
        if (message->type != NAT_VALUE_STRING) {
            message = nat_send(env, message, "inspect", 0, NULL, NULL);
        }
        self->message = message->str;
    }
    return self;
}

NatObject *Exception_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(self->type == NAT_VALUE_EXCEPTION);
    NatObject *str = nat_string(env, "#<");
    assert(self->class);
    nat_string_append(str, Class_inspect(env, self->class, 0, NULL, NULL, NULL)->str);
    nat_string_append(str, ": ");
    nat_string_append(str, self->message);
    nat_string_append_char(str, '>');
    return str;
}
