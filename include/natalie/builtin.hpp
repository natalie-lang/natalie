#pragma once

#include <fcntl.h>

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

#define NAT_KERNEL_INIT(module)                                                            \
    module->define_method(env, "puts", Kernel_puts);                                       \
    module->define_method(env, "print", Kernel_print);                                     \
    module->define_method(env, "p", Kernel_p);                                             \
    module->define_method(env, "inspect", Kernel_inspect);                                 \
    module->define_method(env, "object_id", Kernel_object_id);                             \
    module->define_method(env, "===", Kernel_equal);                                       \
    module->define_method(env, "eql?", Kernel_equal);                                      \
    module->define_method(env, "class", Kernel_class);                                     \
    module->define_method(env, "singleton_class", Kernel_singleton_class);                 \
    module->define_method(env, "instance_variables", Kernel_instance_variables);           \
    module->define_method(env, "instance_variable_get", Kernel_instance_variable_get);     \
    module->define_method(env, "instance_variable_set", Kernel_instance_variable_set);     \
    module->define_method(env, "raise", Kernel_raise);                                     \
    module->define_method(env, "exit", Kernel_exit);                                       \
    module->define_method(env, "at_exit", Kernel_at_exit);                                 \
    module->define_method(env, "respond_to?", Kernel_respond_to);                          \
    module->define_method(env, "dup", Kernel_dup);                                         \
    module->define_method(env, "methods", Kernel_methods);                                 \
    module->define_method(env, "public_methods", Kernel_methods);                          \
    module->define_method(env, "is_a?", Kernel_is_a);                                      \
    module->define_method(env, "hash", Kernel_hash);                                       \
    module->define_method(env, "proc", Kernel_proc);                                       \
    module->define_method(env, "lambda", Kernel_lambda);                                   \
    module->define_method(env, "__method__", Kernel_method);                               \
    module->define_method(env, "freeze", Kernel_freeze);                                   \
    module->define_method(env, "nil?", Kernel_is_nil);                                     \
    module->define_method(env, "sleep", Kernel_sleep);                                     \
    module->define_method(env, "define_singleton_method", Kernel_define_singleton_method); \
    module->define_method(env, "tap", Kernel_tap);                                         \
    module->define_method(env, "Array", Kernel_Array);                                     \
    module->define_method(env, "send", Kernel_send);                                       \
    module->define_method(env, "__dir__", Kernel_cur_dir);                                 \
    module->define_method(env, "get_usage", Kernel_get_usage);

Value *Kernel_puts(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_print(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_p(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_object_id(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_equal(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_class(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_singleton_class(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_instance_variables(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_instance_variable_get(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_instance_variable_set(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_raise(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_respond_to(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_dup(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_methods(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_exit(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_at_exit(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_is_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_hash(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_proc(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_lambda(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_freeze(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_is_nil(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_sleep(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_define_singleton_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_tap(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_Array(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_send(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_cur_dir(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Kernel_get_usage(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *main_obj_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_PROCESS_INIT(module) \
    module->define_singleton_method(env, "pid", Process_pid);

Value *Process_pid(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

}
