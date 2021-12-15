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

#include "natalie/array_object.hpp"
#include "natalie/bignum_object.hpp"
#include "natalie/binding_object.hpp"
#include "natalie/block.hpp"
#include "natalie/class_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/env.hpp"
#include "natalie/env_object.hpp"
#include "natalie/exception_object.hpp"
#include "natalie/false_object.hpp"
#include "natalie/fiber_object.hpp"
#include "natalie/file_object.hpp"
#include "natalie/float_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc_module.hpp"
#include "natalie/global_env.hpp"
#include "natalie/hash_builder.hpp"
#include "natalie/hash_object.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/io_object.hpp"
#include "natalie/kernel_module.hpp"
#include "natalie/local_jump_error_type.hpp"
#include "natalie/match_data_object.hpp"
#include "natalie/method.hpp"
#include "natalie/method_object.hpp"
#include "natalie/module_object.hpp"
#include "natalie/nil_object.hpp"
#include "natalie/object.hpp"
#include "natalie/parser_object.hpp"
#include "natalie/proc_object.hpp"
#include "natalie/process_module.hpp"
#include "natalie/random_object.hpp"
#include "natalie/range_object.hpp"
#include "natalie/regexp_object.hpp"
#include "natalie/sexp_object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie/true_object.hpp"
#include "natalie/types.hpp"
#include "natalie/value_ptr.hpp"
#include "natalie/void_p_object.hpp"

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
void print_exception_with_backtrace(Env *env, ExceptionObject *exception);
void handle_top_level_exception(Env *, ExceptionObject *, bool);

ArrayObject *to_ary(Env *env, ValuePtr obj, bool raise_for_non_array);

struct ArgValueByPathOptions {
    ValuePtr value;
    ValuePtr default_value;
    bool splat;
    bool has_kwargs;
    int total_count;
    int default_count;
    bool defaults_on_right;
    int offset_from_end;
};
ValuePtr arg_value_by_path(Env *env, ArgValueByPathOptions options, size_t path_size, ...);

struct ArrayValueByPathOptions {
    ValuePtr value;
    ValuePtr default_value;
    bool splat;
    int offset_from_end;
};
ValuePtr array_value_by_path(Env *env, ArrayValueByPathOptions options, size_t path_size, ...);

HashObject *kwarg_hash(ValuePtr args);
HashObject *kwarg_hash(ArrayObject *args);
ValuePtr kwarg_value_by_name(Env *env, ValuePtr args, const char *name, ValuePtr default_value);
ValuePtr kwarg_value_by_name(Env *env, ArrayObject *args, const char *name, ValuePtr default_value);

ArrayObject *args_to_array(Env *env, size_t argc, ValuePtr *args);
ArrayObject *block_args_to_array(Env *env, size_t signature_size, size_t argc, ValuePtr *args);

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
