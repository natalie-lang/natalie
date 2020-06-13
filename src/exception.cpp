#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Exception_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    ExceptionValue *exception = new ExceptionValue { env, self->as_class() };
    exception->initialize(env, argc, args, block);
    if (exception->message == NULL) exception->message = strdup(self->as_class()->class_name());
    return exception;
}

Value *Exception_initialize(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ExceptionValue *self = self_value->as_exception();
    if (argc == 0) {
        self->message = strdup(self->klass->class_name());
    } else if (argc == 1) {
        Value *message = args[0];
        if (!message->is_string()) {
            message = message->send(env, "inspect");
        }
        self->message = strdup(message->as_string()->c_str());
    }
    return self;
}

Value *Exception_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    ExceptionValue *self = self_value->as_exception();
    StringValue *out = new StringValue { env, "#<" };
    assert(NAT_OBJ_CLASS(self));
    out->append(env, Module_inspect(env, NAT_OBJ_CLASS(self), 0, NULL, NULL)->as_string()->c_str());
    out->append(env, ": ");
    out->append(env, self->message);
    out->append_char(env, '>');
    return out;
}

Value *Exception_message(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    ExceptionValue *self = self_value->as_exception();
    return new StringValue { env, self->message };
}

Value *Exception_backtrace(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    ExceptionValue *self = self_value->as_exception();
    return self->backtrace;
}

}
