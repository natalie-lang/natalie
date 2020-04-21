#pragma once

#include <fcntl.h>

#include "natalie.h"

#define NAT_ARRAY_INIT(klass)                                                \
    nat_define_singleton_method(env, klass, "new", Array_new);               \
    nat_define_singleton_method(env, klass, "[]", Array_square_new);         \
    nat_define_method(env, klass, "inspect", Array_inspect);                 \
    nat_define_method(env, klass, "to_s", Array_inspect);                    \
    nat_define_method(env, klass, "<<", Array_ltlt);                         \
    nat_define_method(env, klass, "+", Array_add);                           \
    nat_define_method(env, klass, "-", Array_sub);                           \
    nat_define_method(env, klass, "[]", Array_ref);                          \
    nat_define_method(env, klass, "[]=", Array_refeq);                       \
    nat_define_method(env, klass, "size", Array_size);                       \
    nat_define_method(env, klass, "any?", Array_any);                        \
    nat_define_method(env, klass, "length", Array_size);                     \
    nat_define_method(env, klass, "==", Array_eqeq);                         \
    nat_define_method(env, klass, "===", Array_eqeq);                        \
    nat_define_method(env, klass, "each", Array_each);                       \
    nat_define_method(env, klass, "each_with_index", Array_each_with_index); \
    nat_define_method(env, klass, "map", Array_map);                         \
    nat_define_method(env, klass, "first", Array_first);                     \
    nat_define_method(env, klass, "last", Array_last);                       \
    nat_define_method(env, klass, "to_ary", Array_to_ary);                   \
    nat_define_method(env, klass, "pop", Array_pop);                         \
    nat_define_method(env, klass, "include?", Array_include);                \
    nat_define_method(env, klass, "sort", Array_sort);                       \
    nat_define_method(env, klass, "join", Array_join);                       \
    nat_define_method(env, klass, "<=>", Array_cmp);                         \
    nat_define_method(env, klass, "to_a", Array_to_a);

NatObject *Array_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_square_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_ltlt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_refeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_any(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_each(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_each_with_index(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_map(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_first(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_last(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_to_ary(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_pop(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_include(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_sort(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_join(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_cmp(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Array_to_a(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

NatObject *BasicObject_not(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *BasicObject_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *BasicObject_neq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *BasicObject_instance_eval(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

NatObject *Class_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Class_superclass(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_COMPARABLE_INIT(module)                        \
    nat_define_method(env, module, "==", Comparable_eqeq); \
    nat_define_method(env, module, "!=", Comparable_neq);  \
    nat_define_method(env, module, "<", Comparable_lt);    \
    nat_define_method(env, module, "<=", Comparable_lte);  \
    nat_define_method(env, module, ">", Comparable_gt);    \
    nat_define_method(env, module, ">=", Comparable_gte);

NatObject *Comparable_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_neq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_lt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_lte(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_gt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Comparable_gte(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_ENCODING_INIT(klass)                                    \
    nat_define_singleton_method(env, klass, "list", Encoding_list); \
    nat_define_method(env, klass, "inspect", Encoding_inspect);     \
    nat_define_method(env, klass, "name", Encoding_name);           \
    nat_define_method(env, klass, "names", Encoding_names);

NatObject *Encoding_list(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Encoding_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Encoding_name(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Encoding_names(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

NatObject *ENV_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *ENV_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *ENV_refeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_ENV_INIT(obj)                                          \
    nat_define_singleton_method(env, obj, "inspect", ENV_inspect); \
    nat_define_singleton_method(env, obj, "[]", ENV_ref);          \
    nat_define_singleton_method(env, obj, "[]=", ENV_refeq);

NatObject *Exception_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Exception_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Exception_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Exception_message(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Exception_backtrace(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_FALSE_CLASS_INIT(klass)                         \
    nat_undefine_singleton_method(env, klass, "new");       \
    nat_define_method(env, klass, "to_s", FalseClass_to_s); \
    nat_define_method(env, klass, "inspect", FalseClass_to_s);

NatObject *FalseClass_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *FalseClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_FILE_INIT(klass)                                                 \
    NatObject *Constants = nat_module(env, "Constants");                     \
    nat_define_method(env, klass, "initialize", File_initialize);            \
    nat_const_set(env, klass, "Constants", Constants);                       \
    nat_const_set(env, klass, "APPEND", nat_integer(env, O_APPEND));         \
    nat_const_set(env, Constants, "APPEND", nat_integer(env, O_APPEND));     \
    nat_const_set(env, klass, "RDONLY", nat_integer(env, O_RDONLY));         \
    nat_const_set(env, Constants, "RDONLY", nat_integer(env, O_RDONLY));     \
    nat_const_set(env, klass, "WRONLY", nat_integer(env, O_WRONLY));         \
    nat_const_set(env, Constants, "WRONLY", nat_integer(env, O_WRONLY));     \
    nat_const_set(env, klass, "TRUNC", nat_integer(env, O_TRUNC));           \
    nat_const_set(env, Constants, "TRUNC", nat_integer(env, O_TRUNC));       \
    nat_const_set(env, klass, "CREAT", nat_integer(env, O_CREAT));           \
    nat_const_set(env, Constants, "CREAT", nat_integer(env, O_CREAT));       \
    nat_const_set(env, klass, "DSYNC", nat_integer(env, O_DSYNC));           \
    nat_const_set(env, Constants, "DSYNC", nat_integer(env, O_DSYNC));       \
    nat_const_set(env, klass, "EXCL", nat_integer(env, O_EXCL));             \
    nat_const_set(env, Constants, "EXCL", nat_integer(env, O_EXCL));         \
    nat_const_set(env, klass, "NOCTTY", nat_integer(env, O_NOCTTY));         \
    nat_const_set(env, Constants, "NOCTTY", nat_integer(env, O_NOCTTY));     \
    nat_const_set(env, klass, "NOFOLLOW", nat_integer(env, O_NOFOLLOW));     \
    nat_const_set(env, Constants, "NOFOLLOW", nat_integer(env, O_NOFOLLOW)); \
    nat_const_set(env, klass, "NONBLOCK", nat_integer(env, O_NONBLOCK));     \
    nat_const_set(env, Constants, "NONBLOCK", nat_integer(env, O_NONBLOCK)); \
    nat_const_set(env, klass, "RDWR", nat_integer(env, O_RDWR));             \
    nat_const_set(env, Constants, "RDWR", nat_integer(env, O_RDWR));         \
    nat_const_set(env, klass, "SYNC", nat_integer(env, O_SYNC));             \
    nat_const_set(env, Constants, "SYNC", nat_integer(env, O_SYNC));

NatObject *File_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_HASH_INIT(klass)                                        \
    nat_define_singleton_method(env, klass, "new", Hash_new);       \
    nat_define_singleton_method(env, klass, "[]", Hash_square_new); \
    nat_define_method(env, klass, "inspect", Hash_inspect);         \
    nat_define_method(env, klass, "to_s", Hash_inspect);            \
    nat_define_method(env, klass, "[]", Hash_ref);                  \
    nat_define_method(env, klass, "[]=", Hash_refeq);               \
    nat_define_method(env, klass, "delete", Hash_delete);           \
    nat_define_method(env, klass, "size", Hash_size);               \
    nat_define_method(env, klass, "==", Hash_eqeq);                 \
    nat_define_method(env, klass, "===", Hash_eqeq);                \
    nat_define_method(env, klass, "each", Hash_each);               \
    nat_define_method(env, klass, "keys", Hash_keys);               \
    nat_define_method(env, klass, "values", Hash_values);           \
    nat_define_method(env, klass, "sort", Hash_sort);               \
    nat_define_method(env, klass, "key?", Hash_is_key);

NatObject *Hash_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_square_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_refeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_delete(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_each(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_keys(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_values(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_sort(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Hash_is_key(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_INTEGER_INIT(klass)                              \
    nat_define_method(env, klass, "to_s", Integer_to_s);     \
    nat_define_method(env, klass, "inspect", Integer_to_s);  \
    nat_define_method(env, klass, "+", Integer_add);         \
    nat_define_method(env, klass, "-", Integer_sub);         \
    nat_define_method(env, klass, "*", Integer_mul);         \
    nat_define_method(env, klass, "/", Integer_div);         \
    nat_define_method(env, klass, "%", Integer_mod);         \
    nat_define_method(env, klass, "**", Integer_pow);        \
    nat_define_method(env, klass, "<=>", Integer_cmp);       \
    nat_define_method(env, klass, "===", Integer_eqeqeq);    \
    nat_define_method(env, klass, "times", Integer_times);   \
    nat_define_method(env, klass, "&", Integer_bitwise_and); \
    nat_define_method(env, klass, "|", Integer_bitwise_or);  \
    nat_define_method(env, klass, "succ", Integer_succ);

NatObject *Integer_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_div(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_mod(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_pow(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_cmp(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_times(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_bitwise_and(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_bitwise_or(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Integer_succ(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_IO_INIT(klass)                                      \
    nat_define_singleton_method(env, klass, "new", IO_new);     \
    nat_define_method(env, klass, "initialize", IO_initialize); \
    nat_define_method(env, klass, "fileno", IO_fileno);         \
    nat_define_method(env, klass, "read", IO_read);             \
    nat_define_method(env, klass, "write", IO_write);           \
    nat_define_method(env, klass, "puts", IO_puts);             \
    nat_define_method(env, klass, "print", IO_print);           \
    nat_define_method(env, klass, "close", IO_close);           \
    nat_define_method(env, klass, "seek", IO_seek);

NatObject *IO_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *IO_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *IO_fileno(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *IO_read(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *IO_write(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *IO_puts(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *IO_print(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *IO_close(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *IO_seek(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_KERNEL_INIT(module)                                                                \
    nat_define_method(env, module, "puts", Kernel_puts);                                       \
    nat_define_method(env, module, "print", Kernel_print);                                     \
    nat_define_method(env, module, "p", Kernel_p);                                             \
    nat_define_method(env, module, "inspect", Kernel_inspect);                                 \
    nat_define_method(env, module, "object_id", Kernel_object_id);                             \
    nat_define_method(env, module, "===", Kernel_equal);                                       \
    nat_define_method(env, module, "eql?", Kernel_equal);                                      \
    nat_define_method(env, module, "class", Kernel_class);                                     \
    nat_define_method(env, module, "singleton_class", Kernel_singleton_class);                 \
    nat_define_method(env, module, "instance_variables", Kernel_instance_variables);           \
    nat_define_method(env, module, "instance_variable_get", Kernel_instance_variable_get);     \
    nat_define_method(env, module, "instance_variable_set", Kernel_instance_variable_set);     \
    nat_define_method(env, module, "raise", Kernel_raise);                                     \
    nat_define_method(env, module, "exit", Kernel_exit);                                       \
    nat_define_method(env, module, "at_exit", Kernel_at_exit);                                 \
    nat_define_method(env, module, "respond_to?", Kernel_respond_to);                          \
    nat_define_method(env, module, "dup", Kernel_dup);                                         \
    nat_define_method(env, module, "methods", Kernel_methods);                                 \
    nat_define_method(env, module, "public_methods", Kernel_methods);                          \
    nat_define_method(env, module, "is_a?", Kernel_is_a);                                      \
    nat_define_method(env, module, "hash", Kernel_hash);                                       \
    nat_define_method(env, module, "proc", Kernel_proc);                                       \
    nat_define_method(env, module, "lambda", Kernel_lambda);                                   \
    nat_define_method(env, module, "__method__", Kernel_method);                               \
    nat_define_method(env, module, "freeze", Kernel_freeze);                                   \
    nat_define_method(env, module, "nil?", Kernel_is_nil);                                     \
    nat_define_method(env, module, "sleep", Kernel_sleep);                                     \
    nat_define_method(env, module, "define_singleton_method", Kernel_define_singleton_method); \
    nat_define_method(env, module, "tap", Kernel_tap);                                         \
    nat_define_method(env, module, "Array", Kernel_Array);                                     \
    nat_define_method(env, module, "send", Kernel_send);

NatObject *Kernel_puts(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_print(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_p(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_object_id(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_equal(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_class(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_singleton_class(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_instance_variables(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_instance_variable_get(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_instance_variable_set(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_raise(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_respond_to(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_dup(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_methods(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_exit(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_at_exit(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_is_a(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_hash(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_proc(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_lambda(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_method(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_freeze(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_is_nil(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_sleep(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_define_singleton_method(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_tap(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_Array(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Kernel_send(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

NatObject *main_obj_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_MATCH_DATA_INIT(klass)                           \
    nat_define_method(env, klass, "size", MatchData_size);   \
    nat_define_method(env, klass, "length", MatchData_size); \
    nat_define_method(env, klass, "to_s", MatchData_to_s);   \
    nat_define_method(env, klass, "[]", MatchData_ref);

NatObject *MatchData_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *MatchData_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *MatchData_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_MODULE_INIT(klass)                                                  \
    nat_define_singleton_method(env, klass, "new", Module_new);                 \
    nat_define_method(env, klass, "inspect", Module_inspect);                   \
    nat_define_method(env, klass, "name", Module_name);                         \
    nat_define_method(env, klass, "===", Module_eqeqeq);                        \
    nat_define_method(env, klass, "ancestors", Module_ancestors);               \
    nat_define_method(env, klass, "attr", Module_attr_reader);                  \
    nat_define_method(env, klass, "attr_reader", Module_attr_reader);           \
    nat_define_method(env, klass, "attr_writer", Module_attr_writer);           \
    nat_define_method(env, klass, "attr_accessor", Module_attr_accessor);       \
    nat_define_method(env, klass, "include", Module_include);                   \
    nat_define_method(env, klass, "included_modules", Module_included_modules); \
    nat_define_method(env, klass, "define_method", Module_define_method);       \
    nat_define_method(env, klass, "class_eval", Module_class_eval);

NatObject *Module_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_name(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_ancestors(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_attr_reader(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_attr_reader_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_attr_writer(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_attr_writer_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_attr_accessor(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_include(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_included_modules(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_define_method(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Module_class_eval(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_NIL_CLASS_INIT(klass)                               \
    nat_undefine_singleton_method(env, klass, "new");           \
    nat_define_method(env, klass, "to_s", NilClass_to_s);       \
    nat_define_method(env, klass, "to_a", NilClass_to_a);       \
    nat_define_method(env, klass, "inspect", NilClass_inspect); \
    nat_define_method(env, klass, "nil?", NilClass_is_nil);

NatObject *NilClass_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *NilClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *NilClass_to_a(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *NilClass_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *NilClass_is_nil(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_PROC_INIT(klass)                                  \
    nat_define_singleton_method(env, klass, "new", Proc_new); \
    nat_define_method(env, klass, "call", Proc_call);         \
    nat_define_method(env, klass, "lambda?", Proc_lambda);

NatObject *Proc_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Proc_call(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Proc_lambda(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_RANGE_INIT(klass)                                         \
    nat_define_singleton_method(env, klass, "new", Range_new);        \
    nat_define_method(env, klass, "begin", Range_begin);              \
    nat_define_method(env, klass, "first", Range_begin);              \
    nat_define_method(env, klass, "end", Range_end);                  \
    nat_define_method(env, klass, "last", Range_end);                 \
    nat_define_method(env, klass, "exclude_end?", Range_exclude_end); \
    nat_define_method(env, klass, "to_a", Range_to_a);                \
    nat_define_method(env, klass, "each", Range_each);                \
    nat_define_method(env, klass, "inspect", Range_inspect);          \
    nat_define_method(env, klass, "==", Range_eqeq);                  \
    nat_define_method(env, klass, "===", Range_eqeqeq);               \
    nat_define_method(env, klass, "include?", Range_eqeqeq);

NatObject *Range_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Range_begin(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Range_end(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Range_exclude_end(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Range_to_a(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Range_each(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Range_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Range_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Range_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_REGEXP_INIT(klass)                                  \
    nat_define_singleton_method(env, klass, "new", Regexp_new); \
    nat_define_method(env, klass, "==", Regexp_eqeq);           \
    nat_define_method(env, klass, "===", Regexp_match);         \
    nat_define_method(env, klass, "inspect", Regexp_inspect);   \
    nat_define_method(env, klass, "=~", Regexp_eqtilde);        \
    nat_define_method(env, klass, "match", Regexp_match);

NatObject *Regexp_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Regexp_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Regexp_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Regexp_eqtilde(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Regexp_match(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_STRING_INIT(klass)                                              \
    nat_define_singleton_method(env, klass, "new", String_new);             \
    nat_class_include(env, klass, Comparable);                              \
    nat_define_method(env, klass, "to_s", String_to_s);                     \
    nat_define_method(env, klass, "inspect", String_inspect);               \
    nat_define_method(env, klass, "<=>", String_cmp);                       \
    nat_define_method(env, klass, "<<", String_ltlt);                       \
    nat_define_method(env, klass, "+", String_add);                         \
    nat_define_method(env, klass, "*", String_mul);                         \
    nat_define_method(env, klass, "==", String_eqeq);                       \
    nat_define_method(env, klass, "===", String_eqeq);                      \
    nat_define_method(env, klass, "=~", String_eqtilde);                    \
    nat_define_method(env, klass, "match", String_match);                   \
    nat_define_method(env, klass, "succ", String_succ);                     \
    nat_define_method(env, klass, "ord", String_ord);                       \
    nat_define_method(env, klass, "bytes", String_bytes);                   \
    nat_define_method(env, klass, "chars", String_chars);                   \
    nat_define_method(env, klass, "size", String_size);                     \
    nat_define_method(env, klass, "encoding", String_encoding);             \
    nat_define_method(env, klass, "encode", String_encode);                 \
    nat_define_method(env, klass, "force_encoding", String_force_encoding); \
    nat_define_method(env, klass, "[]", String_ref);                        \
    nat_define_method(env, klass, "index", String_index);                   \
    nat_define_method(env, klass, "sub", String_sub);                       \
    nat_define_method(env, klass, "to_i", String_to_i);                     \
    nat_define_method(env, klass, "split", String_split);                   \
    nat_define_method(env, klass, "ljust", String_ljust);

NatObject *String_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_ltlt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_cmp(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_eqtilde(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_match(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_succ(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_ord(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_bytes(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_chars(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_encoding(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_encode(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_force_encoding(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_index(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_to_i(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_split(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *String_ljust(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_SYMBOL_INIT(klass)                                \
    nat_define_method(env, klass, "to_s", Symbol_to_s);       \
    nat_define_method(env, klass, "inspect", Symbol_inspect); \
    nat_define_method(env, klass, "to_proc", Symbol_to_proc);

NatObject *Symbol_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Symbol_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Symbol_to_proc(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Symbol_to_proc_block_fn(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_THREAD_INIT(klass)                                          \
    nat_define_singleton_method(env, klass, "new", Thread_new);         \
    nat_define_singleton_method(env, klass, "current", Thread_current); \
    nat_define_method(env, klass, "join", Thread_join);                 \
    nat_define_method(env, klass, "value", Thread_value);               \
    nat_define_method(env, klass, "inspect", Thread_inspect);

NatObject *Thread_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Thread_current(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Thread_join(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Thread_value(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Thread_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#define NAT_TRUE_CLASS_INIT(klass)                         \
    nat_undefine_singleton_method(env, klass, "new");      \
    nat_define_method(env, klass, "to_s", TrueClass_to_s); \
    nat_define_method(env, klass, "inspect", TrueClass_to_s);

NatObject *TrueClass_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *TrueClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
