#pragma once

#include <fcntl.h>

#include "natalie.hpp"

namespace Natalie {

Value *BasicObject_not(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *BasicObject_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *BasicObject_neq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *BasicObject_instance_eval(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

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

#define NAT_FILE_INIT(klass)                                                     \
    Value *Constants = new ModuleValue { env, "Constants" };                     \
    klass->define_method(env, "initialize", File_initialize);                    \
    klass->define_singleton_method(env, "expand_path", File_expand_path);        \
    klass->const_set(env, "Constants", Constants);                               \
    klass->const_set(env, "APPEND", new IntegerValue { env, O_APPEND });         \
    Constants->const_set(env, "APPEND", new IntegerValue { env, O_APPEND });     \
    klass->const_set(env, "RDONLY", new IntegerValue { env, O_RDONLY });         \
    Constants->const_set(env, "RDONLY", new IntegerValue { env, O_RDONLY });     \
    klass->const_set(env, "WRONLY", new IntegerValue { env, O_WRONLY });         \
    Constants->const_set(env, "WRONLY", new IntegerValue { env, O_WRONLY });     \
    klass->const_set(env, "TRUNC", new IntegerValue { env, O_TRUNC });           \
    Constants->const_set(env, "TRUNC", new IntegerValue { env, O_TRUNC });       \
    klass->const_set(env, "CREAT", new IntegerValue { env, O_CREAT });           \
    Constants->const_set(env, "CREAT", new IntegerValue { env, O_CREAT });       \
    klass->const_set(env, "DSYNC", new IntegerValue { env, O_DSYNC });           \
    Constants->const_set(env, "DSYNC", new IntegerValue { env, O_DSYNC });       \
    klass->const_set(env, "EXCL", new IntegerValue { env, O_EXCL });             \
    Constants->const_set(env, "EXCL", new IntegerValue { env, O_EXCL });         \
    klass->const_set(env, "NOCTTY", new IntegerValue { env, O_NOCTTY });         \
    Constants->const_set(env, "NOCTTY", new IntegerValue { env, O_NOCTTY });     \
    klass->const_set(env, "NOFOLLOW", new IntegerValue { env, O_NOFOLLOW });     \
    Constants->const_set(env, "NOFOLLOW", new IntegerValue { env, O_NOFOLLOW }); \
    klass->const_set(env, "NONBLOCK", new IntegerValue { env, O_NONBLOCK });     \
    Constants->const_set(env, "NONBLOCK", new IntegerValue { env, O_NONBLOCK }); \
    klass->const_set(env, "RDWR", new IntegerValue { env, O_RDWR });             \
    Constants->const_set(env, "RDWR", new IntegerValue { env, O_RDWR });         \
    klass->const_set(env, "SYNC", new IntegerValue { env, O_SYNC });             \
    Constants->const_set(env, "SYNC", new IntegerValue { env, O_SYNC });

Value *File_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *File_expand_path(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

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

#define NAT_MATCH_DATA_INIT(klass)                       \
    klass->define_method(env, "size", MatchData_size);   \
    klass->define_method(env, "length", MatchData_size); \
    klass->define_method(env, "to_s", MatchData_to_s);   \
    klass->define_method(env, "[]", MatchData_ref);

Value *MatchData_size(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *MatchData_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *MatchData_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_MODULE_INIT(klass)                                              \
    klass->define_singleton_method(env, "new", Module_new);                 \
    klass->define_method(env, "inspect", Module_inspect);                   \
    klass->define_method(env, "name", Module_name);                         \
    klass->define_method(env, "===", Module_eqeqeq);                        \
    klass->define_method(env, "ancestors", Module_ancestors);               \
    klass->define_method(env, "attr", Module_attr_reader);                  \
    klass->define_method(env, "attr_reader", Module_attr_reader);           \
    klass->define_method(env, "attr_writer", Module_attr_writer);           \
    klass->define_method(env, "attr_accessor", Module_attr_accessor);       \
    klass->define_method(env, "extend", Module_extend);                     \
    klass->define_method(env, "include", Module_include);                   \
    klass->define_method(env, "prepend", Module_prepend);                   \
    klass->define_method(env, "included_modules", Module_included_modules); \
    klass->define_method(env, "define_method", Module_define_method);       \
    klass->define_method(env, "class_eval", Module_class_eval);             \
    klass->define_method(env, "private", Module_private);                   \
    klass->define_method(env, "protected", Module_protected);               \
    klass->define_method(env, "const_defined?", Module_const_defined);      \
    klass->define_method(env, "alias_method", Module_alias_method);         \
    klass->define_method(env, "method_defined?", Module_method_defined);    \
    klass->define_method(env, "private_method_defined?", Module_method_defined);

Value *Module_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_eqeqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_name(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_ancestors(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_attr_reader(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_attr_reader_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_attr_writer(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_attr_writer_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_attr_accessor(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_extend(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_include(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_prepend(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_included_modules(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_define_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_class_eval(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_private(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_protected(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_const_defined(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_alias_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_method_defined(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *Object_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_PROC_INIT(klass)                              \
    klass->define_singleton_method(env, "new", Proc_new); \
    klass->define_method(env, "call", Proc_call);         \
    klass->define_method(env, "lambda?", Proc_lambda);

Value *Proc_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Proc_call(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Proc_lambda(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_RANGE_INIT(klass)                                     \
    klass->define_singleton_method(env, "new", Range_new);        \
    klass->define_method(env, "begin", Range_begin);              \
    klass->define_method(env, "first", Range_begin);              \
    klass->define_method(env, "end", Range_end);                  \
    klass->define_method(env, "last", Range_end);                 \
    klass->define_method(env, "exclude_end?", Range_exclude_end); \
    klass->define_method(env, "to_a", Range_to_a);                \
    klass->define_method(env, "each", Range_each);                \
    klass->define_method(env, "inspect", Range_inspect);          \
    klass->define_method(env, "==", Range_eqeq);                  \
    klass->define_method(env, "===", Range_eqeqeq);               \
    klass->define_method(env, "include?", Range_eqeqeq);

#define NAT_PROCESS_INIT(module) \
    module->define_singleton_method(env, "pid", Process_pid);

Value *Process_pid(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *Range_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Range_begin(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Range_end(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Range_exclude_end(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Range_to_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Range_each(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Range_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Range_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Range_eqeqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_SYMBOL_INIT(klass)                            \
    klass->define_method(env, "to_s", Symbol_to_s);       \
    klass->define_method(env, "inspect", Symbol_inspect); \
    klass->define_method(env, "to_proc", Symbol_to_proc); \
    klass->define_method(env, "<=>", Symbol_cmp);

Value *Symbol_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_to_proc(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_to_proc_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

}
