#pragma once

#include <fcntl.h>

#include "natalie.hpp"

namespace Natalie {

#define NAT_ARRAY_INIT(klass)                                            \
    define_singleton_method(env, klass, "new", Array_new);               \
    define_singleton_method(env, klass, "[]", Array_square_new);         \
    define_method(env, klass, "inspect", Array_inspect);                 \
    define_method(env, klass, "to_s", Array_inspect);                    \
    define_method(env, klass, "<<", Array_ltlt);                         \
    define_method(env, klass, "+", Array_add);                           \
    define_method(env, klass, "-", Array_sub);                           \
    define_method(env, klass, "[]", Array_ref);                          \
    define_method(env, klass, "[]=", Array_refeq);                       \
    define_method(env, klass, "size", Array_size);                       \
    define_method(env, klass, "any?", Array_any);                        \
    define_method(env, klass, "length", Array_size);                     \
    define_method(env, klass, "==", Array_eqeq);                         \
    define_method(env, klass, "===", Array_eqeq);                        \
    define_method(env, klass, "each", Array_each);                       \
    define_method(env, klass, "each_with_index", Array_each_with_index); \
    define_method(env, klass, "map", Array_map);                         \
    define_method(env, klass, "first", Array_first);                     \
    define_method(env, klass, "last", Array_last);                       \
    define_method(env, klass, "to_ary", Array_to_ary);                   \
    define_method(env, klass, "pop", Array_pop);                         \
    define_method(env, klass, "include?", Array_include);                \
    define_method(env, klass, "sort", Array_sort);                       \
    define_method(env, klass, "join", Array_join);                       \
    define_method(env, klass, "<=>", Array_cmp);                         \
    define_method(env, klass, "to_a", Array_to_a);

Value *Array_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_square_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_ltlt(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_add(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_sub(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_refeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_size(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_any(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_each(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_each_with_index(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_map(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_first(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_last(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_to_ary(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_pop(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_include(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_sort(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_join(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Array_to_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *BasicObject_not(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *BasicObject_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *BasicObject_neq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *BasicObject_instance_eval(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *Class_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Class_superclass(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_COMPARABLE_INIT(module)                    \
    define_method(env, module, "==", Comparable_eqeq); \
    define_method(env, module, "!=", Comparable_neq);  \
    define_method(env, module, "<", Comparable_lt);    \
    define_method(env, module, "<=", Comparable_lte);  \
    define_method(env, module, ">", Comparable_gt);    \
    define_method(env, module, ">=", Comparable_gte);

Value *Comparable_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_neq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_lt(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_lte(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_gt(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Comparable_gte(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_ENCODING_INIT(klass)                                \
    define_singleton_method(env, klass, "list", Encoding_list); \
    define_method(env, klass, "inspect", Encoding_inspect);     \
    define_method(env, klass, "name", Encoding_name);           \
    define_method(env, klass, "names", Encoding_names);

Value *Encoding_list(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Encoding_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Encoding_name(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Encoding_names(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *ENV_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *ENV_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *ENV_refeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_ENV_INIT(obj)                                      \
    define_singleton_method(env, obj, "inspect", ENV_inspect); \
    define_singleton_method(env, obj, "[]", ENV_ref);          \
    define_singleton_method(env, obj, "[]=", ENV_refeq);

Value *Exception_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Exception_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Exception_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Exception_message(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Exception_backtrace(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_FALSE_CLASS_INIT(klass)                     \
    undefine_singleton_method(env, klass, "new");       \
    define_method(env, klass, "to_s", FalseClass_to_s); \
    define_method(env, klass, "inspect", FalseClass_to_s);

Value *FalseClass_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *FalseClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_FILE_INIT(klass)                                              \
    Value *Constants = new ModuleValue { env, "Constants" };              \
    define_method(env, klass, "initialize", File_initialize);             \
    define_singleton_method(env, klass, "expand_path", File_expand_path); \
    const_set(env, klass, "Constants", Constants);                        \
    const_set(env, klass, "APPEND", integer(env, O_APPEND));              \
    const_set(env, Constants, "APPEND", integer(env, O_APPEND));          \
    const_set(env, klass, "RDONLY", integer(env, O_RDONLY));              \
    const_set(env, Constants, "RDONLY", integer(env, O_RDONLY));          \
    const_set(env, klass, "WRONLY", integer(env, O_WRONLY));              \
    const_set(env, Constants, "WRONLY", integer(env, O_WRONLY));          \
    const_set(env, klass, "TRUNC", integer(env, O_TRUNC));                \
    const_set(env, Constants, "TRUNC", integer(env, O_TRUNC));            \
    const_set(env, klass, "CREAT", integer(env, O_CREAT));                \
    const_set(env, Constants, "CREAT", integer(env, O_CREAT));            \
    const_set(env, klass, "DSYNC", integer(env, O_DSYNC));                \
    const_set(env, Constants, "DSYNC", integer(env, O_DSYNC));            \
    const_set(env, klass, "EXCL", integer(env, O_EXCL));                  \
    const_set(env, Constants, "EXCL", integer(env, O_EXCL));              \
    const_set(env, klass, "NOCTTY", integer(env, O_NOCTTY));              \
    const_set(env, Constants, "NOCTTY", integer(env, O_NOCTTY));          \
    const_set(env, klass, "NOFOLLOW", integer(env, O_NOFOLLOW));          \
    const_set(env, Constants, "NOFOLLOW", integer(env, O_NOFOLLOW));      \
    const_set(env, klass, "NONBLOCK", integer(env, O_NONBLOCK));          \
    const_set(env, Constants, "NONBLOCK", integer(env, O_NONBLOCK));      \
    const_set(env, klass, "RDWR", integer(env, O_RDWR));                  \
    const_set(env, Constants, "RDWR", integer(env, O_RDWR));              \
    const_set(env, klass, "SYNC", integer(env, O_SYNC));                  \
    const_set(env, Constants, "SYNC", integer(env, O_SYNC));

Value *File_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *File_expand_path(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_HASH_INIT(klass)                                    \
    define_singleton_method(env, klass, "new", Hash_new);       \
    define_singleton_method(env, klass, "[]", Hash_square_new); \
    define_method(env, klass, "inspect", Hash_inspect);         \
    define_method(env, klass, "to_s", Hash_inspect);            \
    define_method(env, klass, "[]", Hash_ref);                  \
    define_method(env, klass, "[]=", Hash_refeq);               \
    define_method(env, klass, "delete", Hash_delete);           \
    define_method(env, klass, "size", Hash_size);               \
    define_method(env, klass, "==", Hash_eqeq);                 \
    define_method(env, klass, "===", Hash_eqeq);                \
    define_method(env, klass, "each", Hash_each);               \
    define_method(env, klass, "keys", Hash_keys);               \
    define_method(env, klass, "values", Hash_values);           \
    define_method(env, klass, "sort", Hash_sort);               \
    define_method(env, klass, "key?", Hash_is_key);

Value *Hash_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_square_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_refeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_delete(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_size(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_each(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_keys(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_values(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_sort(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Hash_is_key(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_INTEGER_INIT(klass)                          \
    define_method(env, klass, "to_s", Integer_to_s);     \
    define_method(env, klass, "inspect", Integer_to_s);  \
    define_method(env, klass, "+", Integer_add);         \
    define_method(env, klass, "-", Integer_sub);         \
    define_method(env, klass, "*", Integer_mul);         \
    define_method(env, klass, "/", Integer_div);         \
    define_method(env, klass, "%", Integer_mod);         \
    define_method(env, klass, "**", Integer_pow);        \
    define_method(env, klass, "<=>", Integer_cmp);       \
    define_method(env, klass, "===", Integer_eqeqeq);    \
    define_method(env, klass, "times", Integer_times);   \
    define_method(env, klass, "&", Integer_bitwise_and); \
    define_method(env, klass, "|", Integer_bitwise_or);  \
    define_method(env, klass, "succ", Integer_succ);

Value *Integer_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_add(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_sub(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_mul(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_div(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_mod(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_pow(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_eqeqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_times(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_bitwise_and(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_bitwise_or(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_succ(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_IO_INIT(klass)                                  \
    define_singleton_method(env, klass, "new", IO_new);     \
    define_method(env, klass, "initialize", IO_initialize); \
    define_method(env, klass, "fileno", IO_fileno);         \
    define_method(env, klass, "read", IO_read);             \
    define_method(env, klass, "write", IO_write);           \
    define_method(env, klass, "puts", IO_puts);             \
    define_method(env, klass, "print", IO_print);           \
    define_method(env, klass, "close", IO_close);           \
    define_method(env, klass, "seek", IO_seek);

Value *IO_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *IO_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *IO_fileno(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *IO_read(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *IO_write(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *IO_puts(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *IO_print(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *IO_close(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *IO_seek(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_KERNEL_INIT(module)                                                            \
    define_method(env, module, "puts", Kernel_puts);                                       \
    define_method(env, module, "print", Kernel_print);                                     \
    define_method(env, module, "p", Kernel_p);                                             \
    define_method(env, module, "inspect", Kernel_inspect);                                 \
    define_method(env, module, "object_id", Kernel_object_id);                             \
    define_method(env, module, "===", Kernel_equal);                                       \
    define_method(env, module, "eql?", Kernel_equal);                                      \
    define_method(env, module, "class", Kernel_class);                                     \
    define_method(env, module, "singleton_class", Kernel_singleton_class);                 \
    define_method(env, module, "instance_variables", Kernel_instance_variables);           \
    define_method(env, module, "instance_variable_get", Kernel_instance_variable_get);     \
    define_method(env, module, "instance_variable_set", Kernel_instance_variable_set);     \
    define_method(env, module, "raise", Kernel_raise);                                     \
    define_method(env, module, "exit", Kernel_exit);                                       \
    define_method(env, module, "at_exit", Kernel_at_exit);                                 \
    define_method(env, module, "respond_to?", Kernel_respond_to);                          \
    define_method(env, module, "dup", Kernel_dup);                                         \
    define_method(env, module, "methods", Kernel_methods);                                 \
    define_method(env, module, "public_methods", Kernel_methods);                          \
    define_method(env, module, "is_a?", Kernel_is_a);                                      \
    define_method(env, module, "hash", Kernel_hash);                                       \
    define_method(env, module, "proc", Kernel_proc);                                       \
    define_method(env, module, "lambda", Kernel_lambda);                                   \
    define_method(env, module, "__method__", Kernel_method);                               \
    define_method(env, module, "freeze", Kernel_freeze);                                   \
    define_method(env, module, "nil?", Kernel_is_nil);                                     \
    define_method(env, module, "sleep", Kernel_sleep);                                     \
    define_method(env, module, "define_singleton_method", Kernel_define_singleton_method); \
    define_method(env, module, "tap", Kernel_tap);                                         \
    define_method(env, module, "Array", Kernel_Array);                                     \
    define_method(env, module, "send", Kernel_send);                                       \
    define_method(env, module, "__dir__", Kernel_cur_dir);

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

Value *main_obj_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_MATCH_DATA_INIT(klass)                       \
    define_method(env, klass, "size", MatchData_size);   \
    define_method(env, klass, "length", MatchData_size); \
    define_method(env, klass, "to_s", MatchData_to_s);   \
    define_method(env, klass, "[]", MatchData_ref);

Value *MatchData_size(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *MatchData_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *MatchData_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_MODULE_INIT(klass)                                              \
    define_singleton_method(env, klass, "new", Module_new);                 \
    define_method(env, klass, "inspect", Module_inspect);                   \
    define_method(env, klass, "name", Module_name);                         \
    define_method(env, klass, "===", Module_eqeqeq);                        \
    define_method(env, klass, "ancestors", Module_ancestors);               \
    define_method(env, klass, "attr", Module_attr_reader);                  \
    define_method(env, klass, "attr_reader", Module_attr_reader);           \
    define_method(env, klass, "attr_writer", Module_attr_writer);           \
    define_method(env, klass, "attr_accessor", Module_attr_accessor);       \
    define_method(env, klass, "include", Module_include);                   \
    define_method(env, klass, "prepend", Module_prepend);                   \
    define_method(env, klass, "included_modules", Module_included_modules); \
    define_method(env, klass, "define_method", Module_define_method);       \
    define_method(env, klass, "class_eval", Module_class_eval);             \
    define_method(env, klass, "private", Module_private);                   \
    define_method(env, klass, "protected", Module_protected);               \
    define_method(env, klass, "const_defined?", Module_const_defined);      \
    define_method(env, klass, "alias_method", Module_alias_method);

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
Value *Module_include(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_prepend(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_included_modules(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_define_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_class_eval(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_private(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_protected(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_const_defined(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Module_alias_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_NIL_CLASS_INIT(klass)                           \
    undefine_singleton_method(env, klass, "new");           \
    define_method(env, klass, "to_s", NilClass_to_s);       \
    define_method(env, klass, "to_a", NilClass_to_a);       \
    define_method(env, klass, "inspect", NilClass_inspect); \
    define_method(env, klass, "nil?", NilClass_is_nil);

Value *NilClass_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_to_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_is_nil(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *Object_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_PROC_INIT(klass)                              \
    define_singleton_method(env, klass, "new", Proc_new); \
    define_method(env, klass, "call", Proc_call);         \
    define_method(env, klass, "lambda?", Proc_lambda);

Value *Proc_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Proc_call(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Proc_lambda(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_RANGE_INIT(klass)                                     \
    define_singleton_method(env, klass, "new", Range_new);        \
    define_method(env, klass, "begin", Range_begin);              \
    define_method(env, klass, "first", Range_begin);              \
    define_method(env, klass, "end", Range_end);                  \
    define_method(env, klass, "last", Range_end);                 \
    define_method(env, klass, "exclude_end?", Range_exclude_end); \
    define_method(env, klass, "to_a", Range_to_a);                \
    define_method(env, klass, "each", Range_each);                \
    define_method(env, klass, "inspect", Range_inspect);          \
    define_method(env, klass, "==", Range_eqeq);                  \
    define_method(env, klass, "===", Range_eqeqeq);               \
    define_method(env, klass, "include?", Range_eqeqeq);

#define NAT_PROCESS_INIT(module) \
    define_singleton_method(env, module, "pid", Process_pid);

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

#define NAT_REGEXP_INIT(klass)                              \
    define_singleton_method(env, klass, "new", Regexp_new); \
    define_method(env, klass, "==", Regexp_eqeq);           \
    define_method(env, klass, "===", Regexp_match);         \
    define_method(env, klass, "inspect", Regexp_inspect);   \
    define_method(env, klass, "=~", Regexp_eqtilde);        \
    define_method(env, klass, "match", Regexp_match);

Value *Regexp_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Regexp_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Regexp_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Regexp_eqtilde(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Regexp_match(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_STRING_INIT(klass)                                          \
    define_singleton_method(env, klass, "new", String_new);             \
    klass->include(env, Comparable);                                    \
    define_method(env, klass, "initialize", String_initialize);         \
    define_method(env, klass, "to_s", String_to_s);                     \
    define_method(env, klass, "inspect", String_inspect);               \
    define_method(env, klass, "<=>", String_cmp);                       \
    define_method(env, klass, "<<", String_ltlt);                       \
    define_method(env, klass, "+", String_add);                         \
    define_method(env, klass, "*", String_mul);                         \
    define_method(env, klass, "==", String_eqeq);                       \
    define_method(env, klass, "===", String_eqeq);                      \
    define_method(env, klass, "=~", String_eqtilde);                    \
    define_method(env, klass, "match", String_match);                   \
    define_method(env, klass, "succ", String_succ);                     \
    define_method(env, klass, "ord", String_ord);                       \
    define_method(env, klass, "bytes", String_bytes);                   \
    define_method(env, klass, "chars", String_chars);                   \
    define_method(env, klass, "size", String_size);                     \
    define_method(env, klass, "encoding", String_encoding);             \
    define_method(env, klass, "encode", String_encode);                 \
    define_method(env, klass, "force_encoding", String_force_encoding); \
    define_method(env, klass, "[]", String_ref);                        \
    define_method(env, klass, "index", String_index);                   \
    define_method(env, klass, "sub", String_sub);                       \
    define_method(env, klass, "to_i", String_to_i);                     \
    define_method(env, klass, "split", String_split);                   \
    define_method(env, klass, "ljust", String_ljust);

Value *String_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_ltlt(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_add(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_mul(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_eqtilde(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_match(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_succ(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_ord(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_bytes(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_chars(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_size(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_encoding(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_encode(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_force_encoding(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_index(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_sub(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_to_i(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_split(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *String_ljust(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_SYMBOL_INIT(klass)                            \
    define_method(env, klass, "to_s", Symbol_to_s);       \
    define_method(env, klass, "inspect", Symbol_inspect); \
    define_method(env, klass, "to_proc", Symbol_to_proc); \
    define_method(env, klass, "<=>", Symbol_cmp);

Value *Symbol_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_to_proc(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_to_proc_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_TRUE_CLASS_INIT(klass)                     \
    undefine_singleton_method(env, klass, "new");      \
    define_method(env, klass, "to_s", TrueClass_to_s); \
    define_method(env, klass, "inspect", TrueClass_to_s);

Value *TrueClass_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *TrueClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

}
