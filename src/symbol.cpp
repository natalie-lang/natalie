#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Symbol_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self->as_symbol()->to_s(env);
}

Value *Symbol_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self->as_symbol()->inspect(env);
}

Value *Symbol_to_proc(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    SymbolValue *self = self_value->as_symbol();
    NAT_ASSERT_ARGC(0);
    Env block_env = Env::new_detatched_block_env(env);
    block_env.var_set("name", 0, true, self);
    Block *proc_block = block_new(&block_env, self, Symbol_to_proc_block_fn);
    return proc_new(env, proc_block);
}

Value *Symbol_to_proc_block_fn(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    SymbolValue *name_obj = env->outer->var_get("name", 0)->as_symbol();
    assert(name_obj);
    const char *name = name_obj->c_str();
    return args[0]->send(env, name);
}

Value *Symbol_cmp(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    SymbolValue *self = self_value->as_symbol();
    NAT_ASSERT_ARGC(1);
    if (!args[0]->is_symbol()) return NAT_NIL;
    SymbolValue *arg = args[0]->as_symbol();
    int diff = strcmp(self->c_str(), arg->c_str());
    int result;
    if (diff < 0) {
        result = -1;
    } else if (diff > 0) {
        result = 1;
    } else {
        result = 0;
    }
    return new IntegerValue { env, result };
}

}
