#pragma once

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "natalie/array_value.hpp"
#include "natalie/class_value.hpp"
#include "natalie/encoding_value.hpp"
#include "natalie/env.hpp"
#include "natalie/exception_value.hpp"
#include "natalie/false_value.hpp"
#include "natalie/global_env.hpp"
#include "natalie/hash_value.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/io_value.hpp"
#include "natalie/match_data_value.hpp"
#include "natalie/module_value.hpp"
#include "natalie/nil_value.hpp"
#include "natalie/proc_value.hpp"
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

struct Block {
    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block) { nullptr };
    Env env;
    Value *self { nullptr };
};

struct Method {
    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block) { nullptr };
    Env env;
    bool undefined { false };
};

bool is_constant_name(const char *name);

const char *find_current_method_name(Env *env);

void int_to_string(int64_t num, char *buf);
void int_to_hex_string(int64_t num, char *buf, bool capitalize);

void define_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block));
void define_method_with_block(Env *env, Value *obj, const char *name, Block *block);
void define_singleton_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block));
void define_singleton_method_with_block(Env *env, Value *obj, const char *name, Block *block);
void undefine_method(Env *env, Value *obj, const char *name);
void undefine_singleton_method(Env *env, Value *obj, const char *name);

ArrayValue *class_ancestors(Env *env, ModuleValue *klass);
bool is_a(Env *env, Value *obj, Value *klass_or_module);
bool is_a(Env *env, Value *obj, ModuleValue *klass_or_module);

const char *defined(Env *env, Value *receiver, const char *name);
Value *defined_obj(Env *env, Value *receiver, const char *name);

Value *send(Env *env, Value *receiver, const char *sym, ssize_t argc, Value **args, Block *block);
void methods(Env *env, ArrayValue *array, ModuleValue *klass);
Method *find_method(ModuleValue *klass, const char *method_name, ModuleValue **matching_class_or_module);
Method *find_method_without_undefined(Value *klass, const char *method_name, ModuleValue **matching_class_or_module);
Value *call_begin(Env *env, Value *self, Value *(*block_fn)(Env *, Value *));
Value *call_method_on_class(Env *env, ClassValue *klass, Value *instance_class, const char *method_name, Value *self, ssize_t argc, Value **args, Block *block);
bool respond_to(Env *env, Value *obj, const char *name);

Block *block_new(Env *env, Value *self, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *));
Value *_run_block_internal(Env *env, Block *the_block, ssize_t argc, Value **args, Block *block);
ProcValue *proc_new(Env *env, Block *block);
ProcValue *to_proc(Env *env, Value *obj);
ProcValue *lambda(Env *env, Block *block);

Value *splat(Env *env, Value *obj);

RegexpValue *regexp_new(Env *env, const char *pattern);
MatchDataValue *matchdata_new(Env *env, OnigRegion *region, StringValue *str_obj);
Value *last_match(Env *env);

RangeValue *range_new(Env *env, Value *begin, Value *end, bool exclude_end);

Value *dup(Env *env, Value *obj);
Value *bool_not(Env *env, Value *val);

void run_at_exit_handlers(Env *env);
void print_exception_with_backtrace(Env *env, ExceptionValue *exception);
void handle_top_level_exception(Env *env, bool run_exit_handlers);

void object_pointer_id(Value *obj, char *buf);
int64_t object_id(Env *env, Value *obj);

ArrayValue *to_ary(Env *env, Value *obj, bool raise_for_non_array);

Value *arg_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, ssize_t path_size, ...);
Value *array_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int offset_from_end, ssize_t path_size, ...);
Value *kwarg_value_by_name(Env *env, Value *args, const char *name, Value *default_value);
Value *kwarg_value_by_name(Env *env, ArrayValue *args, const char *name, Value *default_value);

ArrayValue *args_to_array(Env *env, ssize_t argc, Value **args);
ArrayValue *block_args_to_array(Env *env, ssize_t signature_size, ssize_t argc, Value **args);

Value *eval_class_or_module_body(Env *env, Value *class_or_module, Value *(*fn)(Env *, Value *));

void arg_spread(Env *env, ssize_t argc, Value **args, const char *arrangement, ...);

template <typename T>
void list_prepend(T *list, T item) {
    T next_item = *list;
    *list = item;
    item->next = next_item;
}

}
