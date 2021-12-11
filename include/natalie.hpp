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
#include "natalie/bignum_value.hpp"
#include "natalie/binding_value.hpp"
#include "natalie/block.hpp"
#include "natalie/class_value.hpp"
#include "natalie/encoding_value.hpp"
#include "natalie/env.hpp"
#include "natalie/env_value.hpp"
#include "natalie/exception_value.hpp"
#include "natalie/false_value.hpp"
#include "natalie/fiber_value.hpp"
#include "natalie/file_value.hpp"
#include "natalie/float_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc_module.hpp"
#include "natalie/global_env.hpp"
#include "natalie/hash_builder.hpp"
#include "natalie/hash_value.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/io_value.hpp"
#include "natalie/kernel_module.hpp"
#include "natalie/local_jump_error_type.hpp"
#include "natalie/match_data_value.hpp"
#include "natalie/method.hpp"
#include "natalie/method_value.hpp"
#include "natalie/module_value.hpp"
#include "natalie/nil_value.hpp"
#include "natalie/parser_value.hpp"
#include "natalie/proc_value.hpp"
#include "natalie/process_module.hpp"
#include "natalie/random_value.hpp"
#include "natalie/range_value.hpp"
#include "natalie/regexp_value.hpp"
#include "natalie/sexp_value.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/true_value.hpp"
#include "natalie/types.hpp"
#include "natalie/value.hpp"
#include "natalie/value_ptr.hpp"
#include "natalie/void_p_value.hpp"
#include "natalie/dsl.hpp"

namespace Natalie {

extern const char *ruby_platform;

extern "C" {
#include "onigmo.h"
}

void init_bindings(Env *);

Env *build_top_env();

const char *find_current_method_name(Env *env);

ValuePtr splat(Env *env, ValuePtr obj);

void run_at_exit_handlers(Env *env);
void print_exception_with_backtrace(Env *env, ExceptionValue *exception);
void handle_top_level_exception(Env *, ExceptionValue *, bool);

ArrayValue *to_ary(Env *env, ValuePtr obj, bool raise_for_non_array);

ValuePtr arg_value_by_path(Env *env, ValuePtr value, ValuePtr default_value, bool splat, bool has_kwargs, int total_count, int default_count, bool defaults_on_right, int offset_from_end, size_t path_size, ...);
ValuePtr array_value_by_path(Env *env, ValuePtr value, ValuePtr default_value, bool splat, int offset_from_end, size_t path_size, ...);

HashValue *kwarg_hash(ValuePtr args);
HashValue *kwarg_hash(ArrayValue *args);
ValuePtr kwarg_value_by_name(Env *env, ValuePtr args, const char *name, ValuePtr default_value);
ValuePtr kwarg_value_by_name(Env *env, ArrayValue *args, const char *name, ValuePtr default_value);

ArrayValue *args_to_array(Env *env, size_t argc, ValuePtr *args);
ArrayValue *block_args_to_array(Env *env, size_t signature_size, size_t argc, ValuePtr *args);

void arg_spread(Env *env, size_t argc, ValuePtr *args, const char *arrangement, ...);

std::pair<ValuePtr, ValuePtr> coerce(Env *, ValuePtr, ValuePtr);

char *zero_string(int);

Block *proc_to_block_arg(Env *, ValuePtr);

ValuePtr shell_backticks(Env *, ValuePtr);

FILE *popen2(const char *, const char *, int &);
int pclose2(FILE *, pid_t);

void set_status_object(Env *, int, int);

const String *int_to_hex_string(nat_int_t, bool);

ValuePtr super(Env *, ValuePtr, size_t, ValuePtr *, Block *);

void clean_up_and_exit(int status);

}
