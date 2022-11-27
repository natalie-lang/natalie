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

#include "natalie/args.hpp"
#include "natalie/array_object.hpp"
#include "natalie/backtrace.hpp"
#include "natalie/binding_object.hpp"
#include "natalie/block.hpp"
#include "natalie/class_object.hpp"
#include "natalie/complex_object.hpp"
#include "natalie/constant.hpp"
#include "natalie/dir_object.hpp"
#include "natalie/encoding/ascii_8bit_encoding_object.hpp"
#include "natalie/encoding/us_ascii_encoding_object.hpp"
#include "natalie/encoding/utf32le_encoding_object.hpp"
#include "natalie/encoding/utf8_encoding_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/enumerator/arithmetic_sequence_object.hpp"
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
#include "natalie/native_profiler.hpp"
#include "natalie/nil_object.hpp"
#include "natalie/object.hpp"
#include "natalie/parser_object.hpp"
#include "natalie/proc_object.hpp"
#include "natalie/process_module.hpp"
#include "natalie/random_object.hpp"
#include "natalie/range_object.hpp"
#include "natalie/rational_object.hpp"
#include "natalie/regexp_object.hpp"
#include "natalie/sexp_object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/string_upto_iterator.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie/time_object.hpp"
#include "natalie/true_object.hpp"
#include "natalie/types.hpp"
#include "natalie/unbound_method_object.hpp"
#include "natalie/value.hpp"
#include "natalie/void_p_object.hpp"

namespace Natalie {

extern const char *ruby_platform;
extern const char *ruby_release_date;
extern const char *ruby_revision;

extern "C" {
#include "onigmo.h"
}

void init_bindings(Env *);

Env *build_top_env();

const char *find_current_method_name(Env *env);

Value splat(Env *env, Value obj);
Value is_case_equal(Env *env, Value case_value, Value when_value, bool is_splat);

void run_at_exit_handlers(Env *env);
void print_exception_with_backtrace(Env *env, ExceptionObject *exception);
void handle_top_level_exception(Env *, ExceptionObject *, bool);

ArrayObject *to_ary(Env *env, Value obj, bool raise_for_non_array);
Value to_ary_for_masgn(Env *env, Value obj);

struct ArgValueByPathOptions {
    TM::Vector<Value> &value;
    Value default_value;
    bool splat;
    bool has_kwargs;
    int total_count;
    int default_count;
    bool defaults_on_right;
    int offset_from_end;
};

void arg_spread(Env *env, Args args, const char *arrangement, ...);

enum class CoerceInvalidReturnValueMode {
    Raise,
    Allow,
};

std::pair<Value, Value> coerce(Env *, Value, Value, CoerceInvalidReturnValueMode = CoerceInvalidReturnValueMode::Raise);

Block *to_block(Env *, Value);
inline Block *to_block(Env *, Block *block) { return block; }

Value shell_backticks(Env *, Value);

FILE *popen2(const char *, const char *, int &);
int pclose2(FILE *, pid_t);

void set_status_object(Env *, int, int);

Value super(Env *, Value, Args, Block *);

void clean_up_and_exit(int status);

inline Value find_top_level_const(Env *env, SymbolObject *name) {
    return GlobalEnv::the()->Object()->const_find(env, name);
}

inline Value fetch_nested_const(std::initializer_list<SymbolObject *> names) {
    Value c = GlobalEnv::the()->Object();
    for (auto name : names)
        c = c->const_fetch(name);
    return c;
}

Value bool_object(bool b);

int hex_char_to_decimal_value(char c);

template <typename T>
void dbg(T val) {
    dbg("{v}", val);
}

template <typename... Args>
void dbg(const char *fmt, Args... args) {
    auto out = StringObject::format(fmt, args...);
    puts(out->c_str());
}

}
