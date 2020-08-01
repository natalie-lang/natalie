#pragma once

#include "natalie.hpp"

namespace Natalie {

#define NAT_COMPARABLE_INIT(module)                    \
    module->define_method(env, "==", Comparable_eqeq); \
    module->define_method(env, "!=", Comparable_neq);  \
    module->define_method(env, "<", Comparable_lt);    \
    module->define_method(env, "<=", Comparable_lte);  \
    module->define_method(env, ">", Comparable_gt);    \
    module->define_method(env, ">=", Comparable_gte);

Value *Comparable_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_neq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_lt(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_lte(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_gt(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_gte(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *ENV_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *ENV_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *ENV_refeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_ENV_INIT(obj)                                      \
    obj->define_singleton_method(env, "inspect", ENV_inspect); \
    obj->define_singleton_method(env, "[]", ENV_ref);          \
    obj->define_singleton_method(env, "[]=", ENV_refeq);

Value *main_obj_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_PROCESS_INIT(module) \
    module->define_singleton_method(env, "pid", Process_pid);

Value *Process_pid(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

}
