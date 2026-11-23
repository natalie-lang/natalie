#include "natalie.hpp"

using namespace Natalie;

Value init_fixture(Env *env, Value self) {
    env->global_set("$llvm_extension_initialized"_s, Value::integer(1));
    return Value::nil();
}

Value LlvmFixture_subtract(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 2);
    return args[0].send(env, "-"_s, { args[1] });
}

Value LlvmFixture_yield(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);
    env->ensure_block_given(block);
    return block->run(env, { args[0] });
}

double llvm_fixture_scale(double number, int factor) {
    return number * factor;
}

Value LlvmFixture_allocate(Env *env, Value self) {
    env->global_set("$llvm_allocated"_s, Value::True());
    return self;
}

Value LlvmFixture_fail(Env *env, Value self, Args &&args, Block *block) {
    env->raise("RuntimeError", "native extension failure");
}
