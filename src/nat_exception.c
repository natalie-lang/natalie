#include "natalie.h"
#include "nat_exception.h"
#include "nat_module.h"
#include "nat_object.h"

NatObject *Exception_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
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
        if (NAT_TYPE(message) != NAT_VALUE_STRING) {
            message = nat_send(env, message, "inspect", 0, NULL, NULL);
        }
        self->message = message->str;
    }
    return self;
}

NatObject *Exception_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    NatObject *str = nat_string(env, "#<");
    assert(self->klass);
    nat_string_append(str, Module_inspect(env, self->klass, 0, NULL, NULL, NULL)->str);
    nat_string_append(str, ": ");
    nat_string_append(str, self->message);
    nat_string_append_char(str, '>');
    return str;
}

NatObject *Exception_message(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    return nat_string(env, self->message);
}

NatObject *Exception_backtrace(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    return self->backtrace;
}
