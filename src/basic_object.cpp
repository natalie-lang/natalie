#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *BasicObject_not(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    if (self->is_truthy()) {
        return env->false_obj();
    } else {
        return env->true_obj();
    }
}

Value *BasicObject_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (self == arg) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

Value *BasicObject_neq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    return BasicObject_not(env, self->send(env, "==", 1, &arg, nullptr), 0, nullptr, nullptr);
}

Value *BasicObject_instance_eval(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    if (argc > 0 || !block) {
        NAT_RAISE(env, "ArgumentError", "Natalie only supports instance_eval with a block");
    }
    Value *self_for_eval = self;
    // I *think* this is right... instance_eval, when called on a class/module,
    // evals with self set to the singleton class
    if (self->type() == Value::Type::Class || self->type() == Value::Type::Module) {
        self_for_eval = self->singleton_class(env);
    }
    block->set_self(self_for_eval);
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
    return env->nil_obj();
}

}
