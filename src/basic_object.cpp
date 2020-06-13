#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *BasicObject_not(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    if (self->is_truthy()) {
        return NAT_FALSE;
    } else {
        return NAT_TRUE;
    }
}

Value *BasicObject_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (self == arg) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
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
    Env e = Env::new_block_env(&block->env, env);
    Value *self_for_eval = self;
    // I *think* this is right... instance_eval, when called on a class/module,
    // evals with self set to the singleton class
    if (NAT_TYPE(self) == Value::Type::Class || NAT_TYPE(self) == Value::Type::Module) {
        self_for_eval = self->singleton_class(env);
    }
    return block->fn(&e, self_for_eval, 0, NULL, NULL);
}

}
