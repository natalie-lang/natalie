#pragma once

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utility>

#include "natalie/array_value.hpp"
#include "natalie/block.hpp"
#include "natalie/class_value.hpp"
#include "natalie/encoding_value.hpp"
#include "natalie/env.hpp"
#include "natalie/env_value.hpp"
#include "natalie/exception_value.hpp"
#include "natalie/false_value.hpp"
#include "natalie/file_value.hpp"
#include "natalie/float_value.hpp"
#include "natalie/global_env.hpp"
#include "natalie/hash_value.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/io_value.hpp"
#include "natalie/kernel_module.hpp"
#include "natalie/match_data_value.hpp"
#include "natalie/method.hpp"
#include "natalie/module_value.hpp"
#include "natalie/nil_value.hpp"
#include "natalie/parser_value.hpp"
#include "natalie/proc_value.hpp"
#include "natalie/process_module.hpp"
#include "natalie/range_value.hpp"
#include "natalie/regexp_value.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/true_value.hpp"
#include "natalie/value.hpp"
#include "natalie/void_p_value.hpp"

namespace Natalie {

extern "C" {
#include "hashmap.h"
#include "onigmo.h"
}

void init_bindings(Env *);

bool is_constant_name(const char *name);
bool is_global_name(const char *name);
bool is_ivar_name(const char *name);

const char *find_current_method_name(Env *env);

void int_to_string(int64_t num, char *buf);
void int_to_hex_string(int64_t num, char *buf, bool capitalize);

Value *call_begin(Env *, Value *, MethodFnPtr, ssize_t, Value **, Block *);

Value *splat(Env *env, Value *obj);

void run_at_exit_handlers(Env *env);
void print_exception_with_backtrace(Env *env, ExceptionValue *exception);
void handle_top_level_exception(Env *, ExceptionValue *, bool);

ArrayValue *to_ary(Env *env, Value *obj, bool raise_for_non_array);

Value *arg_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, ssize_t path_size, ...);
Value *array_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int offset_from_end, ssize_t path_size, ...);
Value *kwarg_value_by_name(Env *env, Value *args, const char *name, Value *default_value);
Value *kwarg_value_by_name(Env *env, ArrayValue *args, const char *name, Value *default_value);

ArrayValue *args_to_array(Env *env, ssize_t argc, Value **args);
ArrayValue *block_args_to_array(Env *env, ssize_t signature_size, ssize_t argc, Value **args);

void arg_spread(Env *env, ssize_t argc, Value **args, const char *arrangement, ...);

template <typename T>
void list_prepend(T *list, T item) {
    T next_item = *list;
    *list = item;
    item->next = next_item;
}

std::pair<Value *, Value *> coerce(Env *, Value *, Value *);

char *zero_string(int);

Block *proc_to_block_arg(Env *, Value *);

}
