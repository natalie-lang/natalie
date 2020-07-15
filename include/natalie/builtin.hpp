#pragma once

#include <fcntl.h>

#include "natalie.hpp"

namespace Natalie {

#define NAT_ARRAY_INIT(klass)                                            \
    klass->define_singleton_method(env, "new", Array_new);               \
    klass->define_singleton_method(env, "[]", Array_square_new);         \
    klass->define_method(env, "inspect", Array_inspect);                 \
    klass->define_method(env, "to_s", Array_inspect);                    \
    klass->define_method(env, "<<", Array_ltlt);                         \
    klass->define_method(env, "+", Array_add);                           \
    klass->define_method(env, "-", Array_sub);                           \
    klass->define_method(env, "[]", Array_ref);                          \
    klass->define_method(env, "[]=", Array_refeq);                       \
    klass->define_method(env, "size", Array_size);                       \
    klass->define_method(env, "any?", Array_any);                        \
    klass->define_method(env, "length", Array_size);                     \
    klass->define_method(env, "==", Array_eqeq);                         \
    klass->define_method(env, "===", Array_eqeq);                        \
    klass->define_method(env, "each", Array_each);                       \
    klass->define_method(env, "each_with_index", Array_each_with_index); \
    klass->define_method(env, "map", Array_map);                         \
    klass->define_method(env, "first", Array_first);                     \
    klass->define_method(env, "last", Array_last);                       \
    klass->define_method(env, "to_ary", Array_to_ary);                   \
    klass->define_method(env, "pop", Array_pop);                         \
    klass->define_method(env, "include?", Array_include);                \
    klass->define_method(env, "sort", Array_sort);                       \
    klass->define_method(env, "join", Array_join);                       \
    klass->define_method(env, "<=>", Array_cmp);                         \
    klass->define_method(env, "to_a", Array_to_a);

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

#define NAT_ENCODING_INIT(klass)                                \
    klass->define_singleton_method(env, "list", Encoding_list); \
    klass->define_method(env, "inspect", Encoding_inspect);     \
    klass->define_method(env, "name", Encoding_name);           \
    klass->define_method(env, "names", Encoding_names);

Value *Encoding_list(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Encoding_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Encoding_name(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Encoding_names(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

Value *ENV_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *ENV_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *ENV_refeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_ENV_INIT(obj)                                      \
    obj->define_singleton_method(env, "inspect", ENV_inspect); \
    obj->define_singleton_method(env, "[]", ENV_ref);          \
    obj->define_singleton_method(env, "[]=", ENV_refeq);

Value *Exception_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Exception_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Exception_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Exception_message(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Exception_backtrace(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_FALSE_CLASS_INIT(klass)                     \
    klass->undefine_singleton_method(env, "new");       \
    klass->define_method(env, "to_s", FalseClass_to_s); \
    klass->define_method(env, "inspect", FalseClass_to_s);

Value *FalseClass_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *FalseClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

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

#define NAT_HASH_INIT(klass)                                    \
    klass->define_singleton_method(env, "new", Hash_new);       \
    klass->define_singleton_method(env, "[]", Hash_square_new); \
    klass->define_method(env, "inspect", Hash_inspect);         \
    klass->define_method(env, "to_s", Hash_inspect);            \
    klass->define_method(env, "[]", Hash_ref);                  \
    klass->define_method(env, "[]=", Hash_refeq);               \
    klass->define_method(env, "delete", Hash_delete);           \
    klass->define_method(env, "size", Hash_size);               \
    klass->define_method(env, "==", Hash_eqeq);                 \
    klass->define_method(env, "===", Hash_eqeq);                \
    klass->define_method(env, "each", Hash_each);               \
    klass->define_method(env, "keys", Hash_keys);               \
    klass->define_method(env, "values", Hash_values);           \
    klass->define_method(env, "sort", Hash_sort);               \
    klass->define_method(env, "key?", Hash_is_key);

#define NAT_FLOAT_INIT(klass)                               \
    klass->define_method(env, "%", Float_mod);              \
    klass->define_method(env, "*", Float_mul);              \
    klass->define_method(env, "**", Float_pow);             \
    klass->define_method(env, "+", Float_add);              \
    klass->define_method(env, "+-", Float_uplus);           \
    klass->define_method(env, "+@", Float_uplus);           \
    klass->define_method(env, "-", Float_sub);              \
    klass->define_method(env, "-@", Float_uminus);          \
    klass->define_method(env, "/", Float_div);              \
    klass->define_method(env, "<", Float_lt);               \
    klass->define_method(env, "<=", Float_lte);             \
    klass->define_method(env, "<=>", Float_cmp);            \
    klass->define_method(env, "==", Float_eqeq);            \
    klass->define_method(env, "===", Float_eqeq);           \
    klass->define_method(env, ">", Float_gt);               \
    klass->define_method(env, ">=", Float_gte);             \
    klass->define_method(env, "abs", Float_abs);            \
    klass->define_method(env, "ceil", Float_ceil);          \
    klass->define_method(env, "coerce", Float_coerce);      \
    klass->define_method(env, "divmod", Float_divmod);      \
    klass->define_method(env, "eql?", Float_eql);           \
    klass->define_method(env, "fdiv", Float_div);           \
    klass->define_method(env, "finite?", Float_finite);     \
    klass->define_method(env, "floor", Float_floor);        \
    klass->define_method(env, "infinite?", Float_infinite); \
    klass->define_method(env, "inspect", Float_to_s);       \
    klass->define_method(env, "nan?", Float_nan);           \
    klass->define_method(env, "quo", Float_div);            \
    klass->define_method(env, "to_i", Float_to_i);          \
    klass->define_method(env, "to_s", Float_to_s);          \
    klass->define_method(env, "zero?", Float_zero);

Value *Float_abs(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_add(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_ceil(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_coerce(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_div(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_divmod(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_eql(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_finite(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_floor(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_gte(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_gt(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_infinite(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_lte(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_lt(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_mod(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_mul(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_nan(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_neg(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_pow(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_sub(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_to_i(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_uminus(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_uplus(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Float_zero(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

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
    klass->define_method(env, "abs", Integer_abs);       \
    klass->define_method(env, "coerce", Integer_coerce); \
    klass->define_method(env, "eql?", Integer_eql);      \
    klass->define_method(env, "inspect", Integer_to_s);  \
    klass->define_method(env, "+", Integer_add);         \
    klass->define_method(env, "&", Integer_bitwise_and); \
    klass->define_method(env, "|", Integer_bitwise_or);  \
    klass->define_method(env, "<=>", Integer_cmp);       \
    klass->define_method(env, "/", Integer_div);         \
    klass->define_method(env, "===", Integer_eqeqeq);    \
    klass->define_method(env, "%", Integer_mod);         \
    klass->define_method(env, "*", Integer_mul);         \
    klass->define_method(env, "**", Integer_pow);        \
    klass->define_method(env, "-", Integer_sub);         \
    klass->define_method(env, "succ", Integer_succ);     \
    klass->define_method(env, "times", Integer_times);   \
    klass->define_method(env, "to_i", Integer_to_i);     \
    klass->define_method(env, "to_s", Integer_to_s);

Value *Integer_abs(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_add(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_bitwise_and(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_bitwise_or(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_coerce(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_div(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_eqeqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_eql(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_mod(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_mul(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_pow(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_sub(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_succ(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_times(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_to_i(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Integer_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_IO_INIT(klass)                                  \
    klass->define_singleton_method(env, "new", IO_new);     \
    klass->define_method(env, "initialize", IO_initialize); \
    klass->define_method(env, "fileno", IO_fileno);         \
    klass->define_method(env, "read", IO_read);             \
    klass->define_method(env, "write", IO_write);           \
    klass->define_method(env, "puts", IO_puts);             \
    klass->define_method(env, "print", IO_print);           \
    klass->define_method(env, "close", IO_close);           \
    klass->define_method(env, "seek", IO_seek);

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
    klass->define_method(env, "include", Module_include);                   \
    klass->define_method(env, "prepend", Module_prepend);                   \
    klass->define_method(env, "included_modules", Module_included_modules); \
    klass->define_method(env, "define_method", Module_define_method);       \
    klass->define_method(env, "class_eval", Module_class_eval);             \
    klass->define_method(env, "private", Module_private);                   \
    klass->define_method(env, "protected", Module_protected);               \
    klass->define_method(env, "const_defined?", Module_const_defined);      \
    klass->define_method(env, "alias_method", Module_alias_method);

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
    klass->undefine_singleton_method(env, "new");           \
    klass->define_method(env, "to_s", NilClass_to_s);       \
    klass->define_method(env, "to_a", NilClass_to_a);       \
    klass->define_method(env, "to_i", NilClass_to_i);       \
    klass->define_method(env, "inspect", NilClass_inspect); \
    klass->define_method(env, "nil?", NilClass_is_nil);

Value *NilClass_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_to_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_to_i(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *NilClass_is_nil(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

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

#define NAT_REGEXP_INIT(klass)                              \
    klass->define_singleton_method(env, "new", Regexp_new); \
    klass->define_method(env, "==", Regexp_eqeq);           \
    klass->define_method(env, "===", Regexp_match);         \
    klass->define_method(env, "inspect", Regexp_inspect);   \
    klass->define_method(env, "=~", Regexp_eqtilde);        \
    klass->define_method(env, "match", Regexp_match);

Value *Regexp_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Regexp_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Regexp_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Regexp_eqtilde(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Regexp_match(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_STRING_INIT(klass)                                          \
    klass->define_singleton_method(env, "new", String_new);             \
    klass->include(env, Comparable);                                    \
    klass->define_method(env, "initialize", String_initialize);         \
    klass->define_method(env, "to_s", String_to_s);                     \
    klass->define_method(env, "inspect", String_inspect);               \
    klass->define_method(env, "<=>", String_cmp);                       \
    klass->define_method(env, "<<", String_ltlt);                       \
    klass->define_method(env, "+", String_add);                         \
    klass->define_method(env, "*", String_mul);                         \
    klass->define_method(env, "==", String_eqeq);                       \
    klass->define_method(env, "===", String_eqeq);                      \
    klass->define_method(env, "=~", String_eqtilde);                    \
    klass->define_method(env, "match", String_match);                   \
    klass->define_method(env, "succ", String_succ);                     \
    klass->define_method(env, "ord", String_ord);                       \
    klass->define_method(env, "bytes", String_bytes);                   \
    klass->define_method(env, "chars", String_chars);                   \
    klass->define_method(env, "size", String_size);                     \
    klass->define_method(env, "encoding", String_encoding);             \
    klass->define_method(env, "encode", String_encode);                 \
    klass->define_method(env, "force_encoding", String_force_encoding); \
    klass->define_method(env, "[]", String_ref);                        \
    klass->define_method(env, "index", String_index);                   \
    klass->define_method(env, "sub", String_sub);                       \
    klass->define_method(env, "to_i", String_to_i);                     \
    klass->define_method(env, "split", String_split);                   \
    klass->define_method(env, "ljust", String_ljust);

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
    klass->define_method(env, "to_s", Symbol_to_s);       \
    klass->define_method(env, "inspect", Symbol_inspect); \
    klass->define_method(env, "to_proc", Symbol_to_proc); \
    klass->define_method(env, "<=>", Symbol_cmp);

Value *Symbol_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_to_proc(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_to_proc_block_fn(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *Symbol_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

#define NAT_TRUE_CLASS_INIT(klass)                     \
    klass->undefine_singleton_method(env, "new");      \
    klass->define_method(env, "to_s", TrueClass_to_s); \
    klass->define_method(env, "inspect", TrueClass_to_s);

Value *TrueClass_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
Value *TrueClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block);

}
