#include "natalie/llvm_runtime.hpp"
#include <cstddef>

using namespace Natalie;

static_assert(sizeof(ParamDescriptor) == 16 && offsetof(ParamDescriptor, name) == 8);
static_assert(static_cast<int>(ParamKind::Req) == 1 && static_cast<int>(ParamKind::Opt) == 2
    && static_cast<int>(ParamKind::Rest) == 3 && static_cast<int>(ParamKind::KeyReq) == 4
    && static_cast<int>(ParamKind::Key) == 5 && static_cast<int>(ParamKind::KeyRest) == 6
    && static_cast<int>(ParamKind::NoKey) == 7 && static_cast<int>(ParamKind::Block) == 8);

extern "C" uint64_t nat_llvm_integer(const char *digits) {
    return nat_llvm_pack(Value::integer(TM::String(digits)));
}

extern "C" uint64_t nat_llvm_float(double number) {
    return nat_llvm_pack(FloatObject::create(number));
}

extern "C" uint64_t nat_llvm_string(const char *bytes, uint64_t size, int status) {
    auto string = StringObject::create(bytes, size, EncodingObject::get(Encoding::UTF_8));
    if (status == 1)
        string->set_chilled(StringObject::Chilled::String);
    else if (status == 2)
        string->freeze();
    return nat_llvm_pack(string);
}

extern "C" uint64_t nat_llvm_symbol(const char *name, uint64_t size) {
    return nat_llvm_pack(SymbolObject::intern(name, size));
}

extern "C" uint64_t nat_llvm_array(uint64_t count, const uint64_t *items) {
    auto array = ArrayObject::create();
    for (uint64_t i = 0; i < count; ++i)
        array->push(nat_llvm_unpack(items[i]));
    return nat_llvm_pack(array);
}

extern "C" uint64_t nat_llvm_object_class() {
    return nat_llvm_pack(GlobalEnv::the()->Object());
}

extern "C" uint64_t nat_llvm_const(Env *env, uint64_t namespace_bits, uint64_t self_bits, const char *name, int strict, int raise) {
    auto search = strict ? Object::ConstLookupSearchMode::Strict : Object::ConstLookupSearchMode::NotStrict;
    auto failure = raise ? Object::ConstLookupFailureMode::Raise : Object::ConstLookupFailureMode::ConstMissing;
    return nat_llvm_pack(Object::const_find_with_autoload(env, nat_llvm_unpack(namespace_bits), nat_llvm_unpack(self_bits), SymbolObject::intern(name), search, failure).value());
}

extern "C" uint64_t nat_llvm_send(Env *env, uint64_t self_bits, uint64_t receiver_bits, const char *name, uint64_t argc, const uint64_t *argv, int public_only, int keywords, const char *file, uint64_t line, uint64_t block_bits) {
    env->set_file(file);
    env->set_line(line);
    TM::Vector<Value> values;
    for (uint64_t i = 0; i < argc; ++i)
        values.push(nat_llvm_unpack(argv[i]));
    Args args(values, keywords);
    auto receiver = nat_llvm_unpack(receiver_bits);
    auto symbol = SymbolObject::intern(name);
    if (public_only)
        return nat_llvm_pack(receiver.public_send(env, symbol, std::move(args), to_block(env, nat_llvm_unpack(block_bits)), nat_llvm_unpack(self_bits)));
    return nat_llvm_pack(receiver.send(env, symbol, std::move(args), to_block(env, nat_llvm_unpack(block_bits))));
}

extern "C" void nat_llvm_set_zero(Env *env, const char *path) {
    env->global_set("$0"_s, StringObject::create(path));
}

extern "C" void nat_llvm_warn(Env *env, uint64_t self_bits, const char *message, int verbose) {
    if (verbose && !GlobalEnv::the()->is_verbose())
        return;
    nat_llvm_unpack(self_bits).send(env, "warn"_s, { StringObject::create(message) });
}

// A captured environment holds the callback separately from Ruby local slots.
// This keeps the C++ calling convention inside the precompiled bridge.
using LlvmFn = uint64_t (*)(Env *, uint64_t, Args *, Block *);

static Value llvm_callback(Env *env, Value self, Args &&args, Block *block) {
    auto address = env->outer()->var_get("llvm_callback", 0).integer().to_nat_int_t();
    auto fn = reinterpret_cast<LlvmFn>(address);
    auto return_point = env->outer()->var_get("llvm_return", 1).integer().to_nat_int_t();
    try {
        return nat_llvm_unpack(fn(env, nat_llvm_pack(self), &args, block));
    } catch (ExceptionObject *exception) {
        auto redo_point = env->outer()->var_get("llvm_redo", 2).integer().to_nat_int_t();
        if (redo_point && exception->is_local_jump_error_with_break_point(redo_point)) {
            FiberObject::current()->set_redo_flag();
            return Value::nil();
        }
        if (return_point && exception->is_local_jump_error_with_break_point(return_point))
            return exception->ivar_get(env, "@exit_value"_s);
        throw;
    }
}

static Block *llvm_block(Env *env, Value self, LlvmFn fn, int arity, bool capture, int64_t return_point, bool has_return = false, int64_t redo_point = 0, const ParamDescriptor *parameters = nullptr) {
    auto closure = Env::create(capture ? Env::create(*env) : nullptr);
    closure->set_lexical_scope(env->lexical_scope());
    closure->set_file(env->file());
    closure->set_line(env->line());
    closure->build_vars(3);
    closure->var_set("llvm_callback", 0, true, Value::integer(reinterpret_cast<intptr_t>(fn)));
    closure->var_set("llvm_return", 1, true, Value::integer(return_point));
    closure->var_set("llvm_redo", 2, true, Value::integer(redo_point));
    return Block::create(*closure, self, llvm_callback, arity, has_return, Block::BlockType::Proc, parameters);
}

extern "C" void nat_llvm_build_vars(Env *env, uint64_t count) {
    env->build_vars(count);
}

// Each closure adds a callback environment between the invocation's locals
// and the captured Ruby environment. Copies share the managed local vector.
static Env *llvm_outer(Env *env, uint64_t depth) {
    while (depth--)
        env = env->outer()->outer();
    return env;
}

extern "C" uint64_t nat_llvm_var_get(Env *env, uint64_t depth, uint64_t index) {
    return nat_llvm_pack(llvm_outer(env, depth)->var_get("llvm_local", index));
}

extern "C" void nat_llvm_var_set(Env *env, uint64_t depth, uint64_t index, uint64_t value) {
    llvm_outer(env, depth)->var_set("llvm_local", index, true, nat_llvm_unpack(value));
}

extern "C" uint64_t nat_llvm_block(Env *env, uint64_t self, LlvmFn fn, int arity, int64_t return_point, int has_return, int64_t redo_point, const ParamDescriptor *parameters) {
    return nat_llvm_pack(ProcObject::from_block_maybe(llvm_block(env, nat_llvm_unpack(self), fn, arity, true, return_point, has_return, redo_point, parameters)));
}

extern "C" void nat_llvm_define_method(Env *env, uint64_t target, const char *name, LlvmFn fn, int arity, const char *file, uint64_t line, int64_t return_point, const ParamDescriptor *parameters) {
    env->set_file(file);
    env->set_line(line);
    auto self = nat_llvm_unpack(target);
    auto block = llvm_block(env, self, fn, arity, false, return_point, false, 0, parameters);
    Object::define_method(env, self, SymbolObject::intern(name), block);
    if (self == GlobalEnv::the()->main_obj())
        self.klass()->set_method_visibility(env, SymbolObject::intern(name), GlobalEnv::the()->top_level_method_visibility());
}

extern "C" uint64_t nat_llvm_arg(Args *args, uint64_t index) {
    return nat_llvm_pack(args->at(index, Value::nil()));
}

extern "C" void nat_llvm_check_args(Env *env, Args *args, int64_t min, int64_t max, int keywords, uint64_t count, const uint64_t *names) {
    Vector<String> required;
    for (uint64_t i = 0; i < count; ++i)
        required.push(nat_llvm_unpack(names[i]).as_symbol()->string());
    if (max < 0)
        args->ensure_argc_at_least(env, min, keywords, required);
    else if (min == max)
        args->ensure_argc_is(env, min, keywords, required);
    else
        args->ensure_argc_between(env, min, max, keywords, required);
}

extern "C" uint64_t nat_llvm_args(Env *env, Args *args, int for_block, int64_t min, int64_t max, int autosplat, int keywords) {
    return nat_llvm_pack(for_block ? args->to_array_for_block(env, min, max, autosplat, keywords) : args->to_array(keywords));
}

extern "C" uint64_t nat_llvm_yield(Env *env, uint64_t argc, const uint64_t *argv, int keywords) {
    TM::Vector<Value> values;
    for (uint64_t i = 0; i < argc; ++i)
        values.push(nat_llvm_unpack(argv[i]));
    return nat_llvm_pack(env->nearest_block()->run(env, Args(values, keywords)));
}

extern "C" uint64_t nat_llvm_global_get(Env *env, const char *name) {
    return nat_llvm_pack(env->global_get(SymbolObject::intern(name)));
}

extern "C" void nat_llvm_global_set(Env *env, const char *name, uint64_t value) {
    env->global_set(SymbolObject::intern(name), nat_llvm_unpack(value));
}

extern "C" uint64_t nat_llvm_array_shift(uint64_t array) {
    return nat_llvm_pack(nat_llvm_unpack(array).as_array()->shift());
}
extern "C" uint64_t nat_llvm_array_pop(uint64_t array) {
    return nat_llvm_pack(nat_llvm_unpack(array).as_array()->pop());
}

extern "C" uint64_t nat_llvm_hash(Env *env, uint64_t count, const uint64_t *items) {
    auto hash = HashObject::create();
    for (uint64_t i = 0; i < count; i += 2)
        hash->put(env, nat_llvm_unpack(items[i]), nat_llvm_unpack(items[i + 1]));
    return nat_llvm_pack(hash);
}

extern "C" uint64_t nat_llvm_range(Env *env, uint64_t begin, uint64_t end, int exclude) {
    return nat_llvm_pack(RangeObject::create(env, nat_llvm_unpack(begin), nat_llvm_unpack(end), exclude));
}

extern "C" uint64_t nat_llvm_ivar_get(Env *env, uint64_t self, const char *name) {
    return nat_llvm_pack(Object::ivar_get(env, nat_llvm_unpack(self), SymbolObject::intern(name)));
}

extern "C" void nat_llvm_ivar_set(Env *env, uint64_t self, const char *name, uint64_t value) {
    Object::ivar_set(env, nat_llvm_unpack(self), SymbolObject::intern(name), nat_llvm_unpack(value));
}

extern "C" void nat_llvm_const_set(Env *env, uint64_t ns, const char *name, uint64_t value, int strict) {
    if (strict)
        Object::const_set(env, nat_llvm_unpack(ns), SymbolObject::intern(name), nat_llvm_unpack(value));
    else
        Object::const_set_not_strict(env, SymbolObject::intern(name), nat_llvm_unpack(value));
}

// Class/module bodies execute synchronously; preserve the callback across nested definitions.
static thread_local LlvmFn llvm_type_body;
static Value llvm_eval_type(Env *env, Value self) {
    Args args;
    return nat_llvm_unpack(llvm_type_body(env, nat_llvm_pack(self), &args, nullptr));
}
extern "C" uint64_t nat_llvm_define_type(Env *env, uint64_t ns, uint64_t self, const char *name, uint64_t superclass, LlvmFn fn, int is_private, int is_module) {
    auto previous = llvm_type_body;
    Defer restore([&] { llvm_type_body = previous; });
    llvm_type_body = fn;
    auto mode = is_private ? Object::ConstLookupSearchMode::StrictPrivate : Object::ConstLookupSearchMode::Strict;
    if (is_module)
        return nat_llvm_pack(Object::define_module(env, nat_llvm_unpack(ns), nat_llvm_unpack(self), SymbolObject::intern(name), mode, llvm_eval_type));
    return nat_llvm_pack(Object::define_class(env, nat_llvm_unpack(ns), nat_llvm_unpack(self), SymbolObject::intern(name), mode, nat_llvm_unpack(superclass), llvm_eval_type));
}

extern "C" uint64_t nat_llvm_load(Env *env, const char *path, LlvmFn fn, int require_once, int64_t return_point) {
    auto filename = SymbolObject::intern(path);
    if (require_once && GlobalEnv::the()->has_file(filename))
        return nat_llvm_pack(Value::False());
    GlobalEnv::the()->add_file(env, filename);
    Env loaded;
    loaded.set_caller(env);
    loaded.set_lexical_scope(GlobalEnv::the()->main_env()->lexical_scope());
    loaded.set_file(path);
    Args args;
    try {
        fn(&loaded, nat_llvm_pack(GlobalEnv::the()->main_obj()), &args, nullptr);
    } catch (ExceptionObject *exception) {
        if (!exception->is_local_jump_error_with_break_point(return_point))
            throw;
    }
    return nat_llvm_pack(Value::True());
}

extern "C" uint64_t nat_llvm_args_empty(Env *env, Args *args, int keywords) {
    return nat_llvm_pack(bool_object(args->size(keywords) == 0));
}

extern "C" uint64_t nat_llvm_args_shift(Env *env, Args *args, int keywords) {
    return nat_llvm_pack(args->shift(env, keywords));
}

extern "C" uint64_t nat_llvm_args_pop(Env *env, Args *args, int keywords) {
    return nat_llvm_pack(args->pop(env, keywords));
}

extern "C" uint64_t nat_llvm_lambda(Env *env, uint64_t proc, int64_t break_point) {
    auto block = to_block(env, nat_llvm_unpack(proc));
    block->set_type(Block::BlockType::Lambda);
    return nat_llvm_pack(ProcObject::create(block, break_point));
}

extern "C" uint64_t nat_llvm_push_block(Env *env, Block *block, int nearest) {
    return nat_llvm_pack(ProcObject::from_block_maybe(nearest ? env->nearest_block(true) : block));
}

extern "C" uint64_t nat_llvm_send_dynamic(Env *env, uint64_t self_bits, uint64_t receiver_bits, const char *name, uint64_t array_bits, Args *forwarded, int public_only, int keywords, const char *file, uint64_t line, uint64_t block_bits, int setter) {
    env->set_file(file);
    env->set_line(line);
    Args args = forwarded ? forwarded->copy() : Args(nat_llvm_unpack(array_bits).as_array(), keywords);
    auto assigned = setter ? args.last() : Value::nil();
    auto receiver = nat_llvm_unpack(receiver_bits);
    auto block = to_block(env, nat_llvm_unpack(block_bits));
    auto symbol = SymbolObject::intern(name);
    auto result = public_only
        ? receiver.public_send(env, symbol, std::move(args), block, nat_llvm_unpack(self_bits))
        : receiver.send(env, symbol, std::move(args), block);
    return nat_llvm_pack(setter ? assigned : result);
}

extern "C" void nat_llvm_array_concat(Env *env, uint64_t array, uint64_t other) {
    auto value = nat_llvm_unpack(other);
    if (!value.is_nil())
        nat_llvm_unpack(array).as_array()->push_splat(env, value);
}

extern "C" void nat_llvm_array_push(uint64_t array, uint64_t value) {
    nat_llvm_unpack(array).as_array()->push(nat_llvm_unpack(value));
}

extern "C" uint64_t nat_llvm_splat(Env *env, uint64_t value) {
    return nat_llvm_pack(splat(env, nat_llvm_unpack(value)));
}

extern "C" uint64_t nat_llvm_to_array(Env *env, uint64_t value) {
    return nat_llvm_pack(to_ary_for_masgn(env, nat_llvm_unpack(value)));
}

extern "C" uint64_t nat_llvm_yield_array(Env *env, uint64_t array, int keywords) {
    return nat_llvm_pack(env->nearest_block()->run(env, Args(nat_llvm_unpack(array).as_array(), keywords)));
}

extern "C" uint64_t nat_llvm_regexp(Env *env, const char *bytes, uint64_t size, int options, int euc_jp) {
    auto encoding = euc_jp ? EncodingObject::get(Encoding::EUC_JP) : nullptr;
    auto regex = RegexpObject::create(env, String(bytes, size), options, encoding);
    regex->freeze();
    return nat_llvm_pack(regex);
}

extern "C" uint64_t nat_llvm_string_regexp(Env *env, uint64_t pattern, int options, int euc_jp) {
    auto encoding = euc_jp ? EncodingObject::get(Encoding::EUC_JP) : nullptr;
    return nat_llvm_pack(RegexpObject::create(env, nat_llvm_unpack(pattern).as_string()->string(), options, encoding));
}

extern "C" void nat_llvm_string_append(Env *env, uint64_t string, uint64_t part) {
    nat_llvm_unpack(string).as_string()->append(nat_llvm_unpack(part).send(env, "to_s"_s));
}

extern "C" uint64_t nat_llvm_case_equal(Env *env, uint64_t value, uint64_t pattern, int splat) {
    return nat_llvm_pack(is_case_equal(env, nat_llvm_unpack(value), nat_llvm_unpack(pattern), splat));
}

// Exception regions keep C++ exception handling in the bridge. LLVM callbacks
// share their caller's Env, so locals and closures retain their Ruby scope.
using LlvmRegionFn = uint64_t (*)(Env *, uint64_t, Args *, Block *, uint64_t *, ExceptionObject *);
extern "C" uint64_t nat_llvm_try(Env *env, uint64_t self, Args *args, Block *block, LlvmRegionFn body, LlvmRegionFn handler, uint64_t *frame, int has_ensure, int discard_catch_result, int64_t retry_point) {
    for (;;) {
        try {
            auto result = body(env, self, args, block, frame, nullptr);
            GlobalEnv::the()->set_rescued(false);
            return result;
        } catch (ExceptionObject *exception) {
            auto previous = env->exception();
            Defer restore([&] { env->set_exception(previous); });
            if (!has_ensure || exception->local_jump_error_type() == LocalJumpErrorType::None)
                env->set_exception(exception);
            try {
                auto result = handler(env, self, args, block, frame, exception);
                GlobalEnv::the()->set_rescued(true);
                if (has_ensure)
                    throw exception;
                return discard_catch_result ? nat_llvm_pack(Value::nil()) : result;
            } catch (ExceptionObject *handler_exception) {
                if (handler_exception->is_local_jump_error_with_break_point(retry_point))
                    continue;
                throw;
            }
        }
    }
}

extern "C" void nat_llvm_jump(Env *env, uint64_t value, int is_return, int64_t point) {
    env->raise_local_jump_error(nat_llvm_unpack(value), is_return ? LocalJumpErrorType::Return : LocalJumpErrorType::Break, point);
}

extern "C" uint64_t nat_llvm_match_break(Env *env, ExceptionObject *exception, int64_t point) {
    if (exception->is_local_jump_error_with_break_point(point))
        return nat_llvm_pack(exception->ivar_get(env, "@exit_value"_s));
    env->raise_exception(exception);
}

extern "C" uint64_t nat_llvm_match_exception(Env *env, ExceptionObject *exception, uint64_t patterns) {
    return nat_llvm_pack(exception->match_rescue_array(env, nat_llvm_unpack(patterns)));
}

extern "C" uint64_t nat_llvm_rescued() {
    return nat_llvm_pack(bool_object(GlobalEnv::the()->rescued()));
}

extern "C" uint64_t nat_llvm_singleton_class(Env *env, uint64_t object) {
    return nat_llvm_pack(Object::singleton_class(env, nat_llvm_unpack(object)));
}

extern "C" uint64_t nat_llvm_with_singleton(Env *env, uint64_t object, LlvmFn fn) {
    auto previous = llvm_type_body;
    Defer restore([&] { llvm_type_body = previous; });
    llvm_type_body = fn;
    auto klass = Object::singleton_class(env, nat_llvm_unpack(object));
    return nat_llvm_pack(klass->eval_body(env, llvm_eval_type));
}

extern "C" void nat_llvm_alias(Env *env, uint64_t self, uint64_t new_name, uint64_t old_name) {
    Object::method_alias(env, nat_llvm_unpack(self), nat_llvm_unpack(new_name), nat_llvm_unpack(old_name));
}

extern "C" uint64_t nat_llvm_super(Env *env, uint64_t receiver, uint64_t array, Args *forwarded, uint64_t block_bits, Block *default_block, int with_block, int keywords) {
    Args args = forwarded
        ? Args(forwarded->original_size(), &tl_current_arg_stack->data()[forwarded->original_start_index()], forwarded->has_keyword_hash())
        : Args(nat_llvm_unpack(array).as_array(), keywords);
    auto block = with_block ? to_block(env, nat_llvm_unpack(block_bits)) : default_block;
    return nat_llvm_pack(super(env, nat_llvm_unpack(receiver), std::move(args), block));
}

static Vector<SymbolObject *> llvm_keyword_names(uint64_t count, const uint64_t *names) {
    Vector<SymbolObject *> result;
    for (uint64_t i = 0; i < count; ++i)
        result.push(nat_llvm_unpack(names[i]).as_symbol());
    return result;
}

extern "C" void nat_llvm_check_keywords(Env *env, Args *args, uint64_t required_count, const uint64_t *required, uint64_t optional_count, const uint64_t *optional, int rest) {
    args->check_keyword_args(env, llvm_keyword_names(required_count, required), llvm_keyword_names(optional_count, optional), static_cast<Args::KeywordRestType>(rest));
}

extern "C" uint64_t nat_llvm_keyword_get(Env *env, Args *args, const char *name) {
    return nat_llvm_pack(args->keyword_arg(env, SymbolObject::intern(name)));
}

extern "C" uint64_t nat_llvm_keyword_present(Env *env, Args *args, const char *name) {
    return nat_llvm_pack(bool_object(args->keyword_arg_present(env, SymbolObject::intern(name))));
}

extern "C" uint64_t nat_llvm_keyword_rest(Env *env, Args *args, uint64_t count, const uint64_t *without) {
    return nat_llvm_pack(args->keyword_arg_rest(env, llvm_keyword_names(count, without)));
}

extern "C" uint64_t nat_llvm_pop_keywords(Args *args) {
    return nat_llvm_pack(args->has_keyword_hash() ? args->pop_keyword_hash() : HashObject::create());
}

extern "C" void nat_llvm_hash_merge(Env *env, uint64_t hash, uint64_t other) {
    auto value = nat_llvm_unpack(other);
    if (!value.is_nil())
        nat_llvm_unpack(hash).as_hash()->merge_in_place(env, { value }, nullptr);
}

// Keep loop branches in LLVM. The bridge only converts scoped local jumps
// into branch statuses after any intervening ensure callbacks have run.
extern "C" int nat_llvm_loop_step(Env *env, uint64_t self, Args *args, Block *block, LlvmRegionFn fn, uint64_t *frame, ExceptionObject *exception, uint64_t *value, int64_t break_point, int64_t next_point, int64_t redo_point) {
    try {
        *value = fn(env, self, args, block, frame, exception);
        return 0;
    } catch (ExceptionObject *jump) {
        if (jump->is_local_jump_error_with_break_point(break_point)) {
            *value = nat_llvm_pack(jump->ivar_get(env, "@exit_value"_s));
            return 1;
        }
        if (jump->is_local_jump_error_with_break_point(next_point))
            return 2;
        if (jump->is_local_jump_error_with_break_point(redo_point))
            return 3;
        throw;
    }
}

extern "C" void nat_llvm_redo() {
    FiberObject::current()->set_redo_flag();
}

extern "C" uint64_t nat_llvm_catch_return(Env *env, uint64_t self, uint64_t (*fn)(Env *, uint64_t), int64_t point) {
    try {
        return fn(env, self);
    } catch (ExceptionObject *exception) {
        if (exception->is_local_jump_error_with_break_point(point))
            return nat_llvm_pack(exception->ivar_get(env, "@exit_value"_s));
        throw;
    }
}

extern "C" uint64_t nat_llvm_cvar_get(Env *env, uint64_t self_bits, const char *name, int nil_default) {
    auto self = nat_llvm_unpack(self_bits);
    auto symbol = SymbolObject::intern(name);
    return nat_llvm_pack(nil_default ? self->cvar_get_maybe(env, symbol).value_or(Value::nil()) : self->cvar_get(env, symbol));
}

extern "C" void nat_llvm_cvar_set(Env *env, uint64_t self, const char *name, uint64_t value) {
    nat_llvm_unpack(self)->cvar_set(env, SymbolObject::intern(name), nat_llvm_unpack(value));
}

extern "C" uint64_t nat_llvm_alias_global(Env *env, uint64_t new_name, uint64_t old_name) {
    return nat_llvm_pack(env->global_alias(nat_llvm_unpack(new_name).to_symbol(env, Value::Conversion::Strict), nat_llvm_unpack(old_name).to_symbol(env, Value::Conversion::Strict)));
}

extern "C" uint64_t nat_llvm_undef_method(Env *env, uint64_t self, uint64_t name) {
    return nat_llvm_pack(Object::undefine_method(env, nat_llvm_unpack(self), nat_llvm_unpack(name).to_symbol(env, Value::Conversion::Strict)));
}

extern "C" uint64_t nat_llvm_is_defined(Env *env, uint64_t self, Args *args, Block *block, LlvmRegionFn fn, uint64_t *frame, const char *type) {
    try {
        fn(env, self, args, block, frame, nullptr);
        auto result = StringObject::create(type);
        result->freeze();
        return nat_llvm_pack(result);
    } catch (ExceptionObject *) {
        return nat_llvm_pack(Value::nil());
    }
}

extern "C" void nat_llvm_method_defined(Env *env, uint64_t receiver, const char *name, int include_all) {
    if (!nat_llvm_unpack(receiver).respond_to(env, SymbolObject::intern(name), include_all))
        throw ExceptionObject::create();
}

extern "C" void nat_llvm_ivar_defined(Env *env, uint64_t self, const char *name) {
    if (!Object::ivar_defined(env, nat_llvm_unpack(self), SymbolObject::intern(name)))
        throw ExceptionObject::create();
}

extern "C" void nat_llvm_global_defined(Env *env, const char *name) {
    if (!env->global_defined(SymbolObject::intern(name)))
        throw ExceptionObject::create();
}

extern "C" uint64_t nat_llvm_last_match(Env *env) {
    return nat_llvm_pack(env->last_match());
}

extern "C" uint64_t nat_llvm_array_empty(uint64_t array) {
    return nat_llvm_pack(bool_object(nat_llvm_unpack(array).as_array()->is_empty()));
}

extern "C" void nat_llvm_hash_put(Env *env, uint64_t hash, uint64_t key, uint64_t value) {
    nat_llvm_unpack(hash).as_hash()->put(env, nat_llvm_unpack(key), nat_llvm_unpack(value));
}

extern "C" uint64_t nat_llvm_hash_delete(Env *env, uint64_t hash, const char *name) {
    return nat_llvm_pack(nat_llvm_unpack(hash).as_hash()->remove(env, SymbolObject::intern(name)).value());
}

extern "C" uint64_t nat_llvm_hash_has_key(Env *env, uint64_t hash, const char *name) {
    return nat_llvm_pack(bool_object(nat_llvm_unpack(hash).as_hash()->has_key(env, SymbolObject::intern(name))));
}

extern "C" uint64_t nat_llvm_dup_object(Env *env, uint64_t object) {
    return nat_llvm_pack(nat_llvm_unpack(object)->duplicate(env));
}
