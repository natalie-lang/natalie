#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Exception_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *exception = Object_new(env, self, argc, args, block);
    exception->type = NAT_VALUE_EXCEPTION;
    if (exception->message == NULL) exception->message = heap_string(self->class_name);
    return exception;
}

Value *Exception_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    if (argc == 0) {
        self->message = heap_string(self->class_name);
    } else if (argc == 1) {
        Value *message = args[0];
        if (NAT_TYPE(message) != NAT_VALUE_STRING) {
            message = send(env, message, "inspect", 0, NULL, NULL);
        }
        self->message = heap_string(message->str);
    }
    return self;
}

Value *Exception_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    Value *str = string(env, "#<");
    assert(NAT_OBJ_CLASS(self));
    string_append(env, str, Module_inspect(env, NAT_OBJ_CLASS(self), 0, NULL, NULL)->str);
    string_append(env, str, ": ");
    string_append(env, str, self->message);
    string_append_char(env, str, '>');
    return str;
}

Value *Exception_message(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    return string(env, self->message);
}

Value *Exception_backtrace(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_EXCEPTION);
    return self->backtrace;
}

}
