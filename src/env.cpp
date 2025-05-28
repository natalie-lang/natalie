#include <stdarg.h>

#include "natalie.hpp"

namespace Natalie {

using namespace TM;

thread_local ExceptionObject *tl_current_exception = nullptr;

void Env::build_vars(size_t size) {
    m_vars = new ManagedVector<Value>(size, Value::nil());
}

bool Env::global_defined(SymbolObject *name) {
    return GlobalEnv::the()->global_defined(this, name);
}

Value Env::global_get(SymbolObject *name) {
    return GlobalEnv::the()->global_get(this, name);
}

Value Env::global_set(SymbolObject *name, Object *obj, bool readonly) {
    return GlobalEnv::the()->global_set(this, name, obj ? Value(obj) : Optional<Value>(), readonly);
}

Value Env::global_set(SymbolObject *name, Optional<Value> val, bool readonly) {
#ifdef NAT_GC_GUARD
    if (val)
        NAT_GC_GUARD_VALUE(val.value());
#endif
    return GlobalEnv::the()->global_set(this, name, val, readonly);
}

Value Env::global_alias(SymbolObject *new_name, SymbolObject *old_name) {
    return GlobalEnv::the()->global_alias(this, new_name, old_name);
}

// Return the file separator `$,` or nil
Value Env::output_file_separator() {
    return global_get("$,"_s);
}

// Return the record separator `$\` or nil
Value Env::output_record_separator() {
    return global_get("$\\"_s);
}

// Return the last line `$_` or nil
Value Env::last_line() {
    return global_get("$_"_s);
}

Value Env::set_last_line(Value val) {
    return global_set("$_"_s, val);
}

Value Env::set_last_lineno(Value val) {
    return global_set("$."_s, val);
}

const Method *Env::current_method() {
    Env *env = this;
    while (!env->method() && env->outer()) {
        env = env->outer();
    }
    return env->method();
}

String Env::build_code_location_name() {
    if (is_main())
        return "<main>";

    if (method())
        return method()->name();

    if (module())
        return module()->backtrace_name();

    // we're in a block, so try to build a string like "block in foo", "block in block in foo", etc.
    if (outer()) {
        auto outer_name = outer()->build_code_location_name();
        return String::format("block in {}", outer_name);
    }
    // fall back to just "block" if we don't know where this block came from
    return String("block");
}

void Env::raise(ClassObject *klass, StringObject *message) {
    ExceptionObject *exception = new ExceptionObject { klass, message };
    this->raise_exception(exception);
}

void Env::raise(ClassObject *klass, String message) {
    raise(klass, StringObject::create(std::move(message)));
}

void Env::raise(const char *class_name, String message) {
    ClassObject *klass = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern(class_name)).as_class();
    ExceptionObject *exception = new ExceptionObject { klass, StringObject::create(std::move(message)) };
    this->raise_exception(exception);
}

void Env::raise_exception(ExceptionObject *exception) {
    tl_current_exception = exception;

    if (!exception->backtrace()) {
        // only build a backtrace the first time the exception is raised (not on a re-raise)
        exception->build_backtrace(this);
    }
    if (!exception->cause()) {
        auto cause = exception_object();
        if (cause.is_exception() && cause != exception)
            exception->set_cause(cause.as_exception());
    }
    throw exception;
}

void Env::raise_key_error(Value receiver, Value key) {
    auto message = StringObject::create(String::format("key not found: {}", key.inspected(this)));
    auto key_error_class = GlobalEnv::the()->Object()->const_fetch("KeyError"_s).as_class();
    ExceptionObject *exception = new ExceptionObject { key_error_class, message };
    exception->ivar_set(this, "@receiver"_s, receiver);
    exception->ivar_set(this, "@key"_s, key);
    this->raise_exception(exception);
}

void Env::raise_local_jump_error(Value exit_value, LocalJumpErrorType type, nat_int_t break_point) {
    auto message = StringObject::create(type == LocalJumpErrorType::Return ? "unexpected return" : "break from proc-closure");
    auto lje_class = find_top_level_const(this, "LocalJumpError"_s).as_class();
    ExceptionObject *exception = new ExceptionObject { lje_class, message };
    exception->set_local_jump_error_type(type);
    exception->ivar_set(this, "@exit_value"_s, exit_value);
    if (break_point != 0)
        exception->set_break_point(break_point);
    if (type == LocalJumpErrorType::Break)
        assert(m_this_block);
    this->raise_exception(exception);
}

void Env::raise_errno() {
    auto SystemCallError = find_top_level_const(this, "SystemCallError"_s);
    ExceptionObject *error = SystemCallError.send(this, "exception"_s, { Value::integer(errno) }).as_exception();
    raise_exception(error);
}

void Env::raise_errno(StringObject *detail) {
    auto SystemCallError = find_top_level_const(this, "SystemCallError"_s);
    ExceptionObject *error = SystemCallError.send(this, "exception"_s, { detail, Value::integer(errno) }).as_exception();
    raise_exception(error);
}

void Env::raise_invalid_byte_sequence_error(const EncodingObject *encoding) {
    auto name = encoding->name()->string();
    raise("ArgumentError", "invalid byte sequence in {}", name);
}

void Env::raise_no_method_error(Value receiver, SymbolObject *name, MethodMissingReason reason) {
    String inspect_module;
    if (receiver.is_nil() || receiver.is_true() || receiver.is_false()) {
        inspect_module = receiver.inspected(this);
    } else if (receiver.is_integer()) {
        inspect_module = String::format("an instance of {}", receiver.klass()->inspect_module());
    } else if (receiver->is_main_object()) {
        inspect_module = "main";
    } else if (receiver.is_class()) {
        inspect_module = String::format("class {}", receiver.inspected(this));
    } else if (receiver.is_module()) {
        inspect_module = String::format("module {}", receiver.inspected(this));
    } else {
        inspect_module = String::format("an instance of {}", receiver.klass()->inspect_module());
    }
    String message;
    switch (reason) {
    case MethodMissingReason::Private:
        message = String::format("private method '{}' called for {}", name->string(), inspect_module);
        break;
    case MethodMissingReason::Protected:
        message = String::format("protected method '{}' called for {}", name->string(), inspect_module);
        break;
    case MethodMissingReason::Undefined:
        message = String::format("undefined method '{}' for {}", name->string(), inspect_module);
        break;
    default:
        NAT_UNREACHABLE();
    }
    auto NoMethodError = find_top_level_const(this, "NoMethodError"_s).as_class();
    ExceptionObject *exception = new ExceptionObject { NoMethodError, StringObject::create(std::move(message)) };
    exception->ivar_set(this, "@receiver"_s, receiver);
    exception->ivar_set(this, "@name"_s, name);
    this->raise_exception(exception);
}

void Env::raise_name_error(SymbolObject *name, String message) {
    auto NameError = find_top_level_const(this, "NameError"_s).as_class();
    ExceptionObject *exception = new ExceptionObject { NameError, StringObject::create(std::move(message)) };
    exception->ivar_set(this, "@name"_s, name);
    this->raise_exception(exception);
}

void Env::raise_name_error(StringObject *name, String message) {
    auto NameError = find_top_level_const(this, "NameError"_s).as_class();
    ExceptionObject *exception = new ExceptionObject { NameError, StringObject::create(std::move(message)) };
    exception->ivar_set(this, "@name"_s, name);
    this->raise_exception(exception);
}

void Env::raise_not_comparable_error(Value lhs, Value rhs) {
    String lhs_class = lhs.klass()->inspect_module();
    String rhs_inspect;
    if (rhs.is_integer() || rhs.is_float() || rhs.is_falsey()) {
        rhs_inspect = rhs.inspected(this);
    } else {
        rhs_inspect = rhs.klass()->inspect_module();
    }

    String message = String::format("comparison of {} with {} failed", lhs_class, rhs_inspect);
    raise("ArgumentError", std::move(message));
}

void Env::raise_type_error(const Value obj, const char *expected) {
    if (obj.is_nil()) {
        auto lowercase_expected = StringObject::create(expected);
        lowercase_expected->downcase_in_place(this);
        raise("TypeError", "no implicit conversion from nil to {}", lowercase_expected->string());
    } else {
        raise("TypeError", "no implicit conversion of {} into {}", obj.klass()->inspect_module(), expected);
    }
}

// This is the same as Env::raise_type_error, but with more consistent error messages.
// We need both methods because CRuby is inconsistent. :-(
void Env::raise_type_error2(const Value obj, const char *expected) {
    if (obj.is_nil())
        raise("TypeError", "no implicit conversion of nil into {}", expected);
    else if (obj.is_true())
        raise("TypeError", "no implicit conversion of true into {}", expected);
    else if (obj.is_false())
        raise("TypeError", "no implicit conversion of false into {}", expected);
    else
        raise("TypeError", "no implicit conversion of {} into {}", obj.klass()->inspect_module(), expected);
}

bool Env::has_catch(Value value) const {
    return (m_catch && Object::equal(value, m_catch.value())) || (m_caller && m_caller->has_catch(value)) || (m_outer && m_outer->has_catch(value));
}

void Env::warn(String message) {
    Value _stderr = global_get("$stderr"_s);
    message = String::format("warning: {}", message);

    if (m_file && m_line) {
        message = String::format("{}:{}: {}", m_file, m_line, message);
    }

    _stderr.send(this, "puts"_s, { StringObject::create(std::move(message)) });
}

void Env::ensure_block_given(Block *block) {
    if (!block) {
        raise("ArgumentError", "called without a block");
    }
}

void Env::ensure_no_missing_keywords(HashObject *kwargs, std::initializer_list<const String> names) {
    if (std::empty(names))
        return;
    Vector<SymbolObject *> missing;
    for (auto name : names) {
        auto symbol = SymbolObject::intern(name);
        if (!kwargs || !kwargs->has_key(this, symbol))
            missing.push(symbol);
    }
    if (missing.is_empty())
        return;
    if (missing.size() == 1)
        raise("ArgumentError", "missing keyword: {}", missing[0]->inspected(this));
    String message { "missing keywords: " };
    message.append(missing[0]->inspected(this));
    for (size_t i = 1; i < missing.size(); i++) {
        message.append(", ");
        message.append(missing[i]->inspected(this));
    }
    raise("ArgumentError", std::move(message));
}

void Env::ensure_no_extra_keywords(HashObject *kwargs) {
    if (!kwargs || kwargs->is_empty())
        return;
    auto it = kwargs->begin();
    if (kwargs->size() == 1)
        raise("ArgumentError", "unknown keyword: {}", it->key.inspected(this));
    String message { "unknown keywords: " };
    message.append(it->key.inspected(this));
    for (it++; it != kwargs->end(); it++) {
        message.append(", ");
        message.append(it->key.inspected(this));
    }
    raise("ArgumentError", std::move(message));
}

Value Env::last_match() {
    Env *env = non_block_env();
    return env->m_match.value_or(Value::nil());
}

bool Env::has_last_match() {
    return non_block_env()->m_match;
}

void Env::set_last_match(MatchDataObject *match) {
    non_block_env()->set_match(match ? Value(match) : Optional<Value>());
}

Value Env::exception_object() {
    auto e = exception();
    if (!e)
        return Value::nil();
    return e;
}

ExceptionObject *Env::exception() {
    auto e = this;
    while (e) {
        if (e->m_exception)
            return e->m_exception;
        e = e->m_caller;
    }
    return nullptr;
}

Backtrace *Env::backtrace() {
    auto backtrace = new Backtrace;

    Env *bt_env = this;
    do {
        if (bt_env->file())
            backtrace->add_item(bt_env, bt_env->file(), bt_env->line());
        bt_env = bt_env->caller();
    } while (bt_env);
    return backtrace;
}

Value Env::var_get(const char *name, size_t index) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!m_vars || index >= m_vars->size())
        return Value::nil();
    return m_vars->at(index);
}

Value Env::var_set(const char *name, size_t index, bool allocate, Value val) {
    NAT_GC_GUARD_VALUE(val);
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    size_t needed = index + 1;
    size_t current_size = m_vars ? m_vars->size() : 0;
    if (needed > current_size) {
        if (allocate) {
            if (!m_vars) {
                build_vars(needed);
            } else {
                m_vars->set_size(needed, Value::nil());
            }
        } else {
            printf("Tried to set a variable without first allocating space for it.\n");
            abort();
        }
    }
    assert(m_vars);
    m_vars->at(index) = val;
    return val;
}

Env *Env::non_block_env() {
    Env *env = this;
    while (env->m_this_block && env->m_this_block->calling_env())
        env = env->m_this_block->calling_env();
    assert(env);
    return env;
}

void Env::visit_children(Visitor &visitor) const {
    Cell::visit_children(visitor);
    visitor.visit(m_vars);
    visitor.visit(m_outer);
    visitor.visit(m_block);
    visitor.visit(m_this_block);
    visitor.visit(m_caller);
    visitor.visit(m_method);
    visitor.visit(m_module);
    if (m_match)
        visitor.visit(m_match.value());
    visitor.visit(m_exception);
    if (m_catch)
        visitor.visit(m_catch.value());
}
}
