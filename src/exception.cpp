#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Exception_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *exception = Object_new(env, self, argc, args, block);
    exception->type = NAT_VALUE_EXCEPTION;
    if (exception->message == NULL) exception->message = heap_string(self->class_name);
    return exception;
}

NatObject *Exception_initialize(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    if (argc == 0) {
        self->message = heap_string(self->class_name);
    } else if (argc == 1) {
        NatObject *message = args[0];
        if (NAT_TYPE(message) != NAT_VALUE_STRING) {
            message = send(env, message, "inspect", 0, NULL, NULL);
        }
        self->message = heap_string(message->str);
    }
    return self;
}

NatObject *Exception_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    NatObject *str = string(env, "#<");
    assert(NAT_OBJ_CLASS(self));
    string_append(env, str, Module_inspect(env, NAT_OBJ_CLASS(self), 0, NULL, NULL)->str);
    string_append(env, str, ": ");
    string_append(env, str, self->message);
    string_append_char(env, str, '>');
    return str;
}

NatObject *Exception_message(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    return string(env, self->message);
}

NatObject *Exception_backtrace(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    return self->backtrace;
}

}
