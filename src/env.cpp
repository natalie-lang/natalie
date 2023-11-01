#include <stdarg.h>

#include "natalie.hpp"

namespace Natalie {

using namespace TM;

void Env::build_vars(size_t size) {
    m_vars = new ManagedVector<Value>(size, NilObject::the());
}

bool Env::global_defined(SymbolObject *name) {
    return GlobalEnv::the()->global_defined(this, name);
}

Value Env::global_get(SymbolObject *name) {
    return GlobalEnv::the()->global_get(this, name);
}

Value Env::global_set(SymbolObject *name, Value val) {
    NAT_GC_GUARD_VALUE(val);
    return GlobalEnv::the()->global_set(this, name, val);
}

Value Env::global_alias(SymbolObject *new_name, SymbolObject *old_name) {
    return GlobalEnv::the()->global_alias(this, new_name, old_name);
}

// Return the file separator `$,` or nil
Value Env::output_file_separator() {
    Value fsep = global_get("$,"_s);
    if (fsep) return fsep;
    return NilObject::the();
}

// Return the record separator `$\` or nil
Value Env::output_record_separator() {
    Value rsep = global_get("$\\"_s);
    if (rsep) return rsep;
    return NilObject::the();
}

// Return the last line `$_` or nil
Value Env::last_line() {
    Value last_line = global_get("$_"_s);
    if (last_line) return last_line;
    return NilObject::the();
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
    raise(klass, new StringObject(std::move(message)));
}

void Env::raise(const char *class_name, String message) {
    ClassObject *klass = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern(class_name))->as_class();
    ExceptionObject *exception = new ExceptionObject { klass, new StringObject { std::move(message) } };
    this->raise_exception(exception);
}

void Env::raise_exception(ExceptionObject *exception) {
    if (!exception->backtrace()) {
        // only build a backtrace the first time the exception is raised (not on a re-raise)
        exception->build_backtrace(this);
    }
    throw exception;
}

void Env::raise_key_error(Value receiver, Value key) {
    auto message = new StringObject { String::format("key not found: {}", key->inspect_str(this)) };
    auto key_error_class = GlobalEnv::the()->Object()->const_fetch("KeyError"_s)->as_class();
    ExceptionObject *exception = new ExceptionObject { key_error_class, message };
    exception->ivar_set(this, "@receiver"_s, receiver);
    exception->ivar_set(this, "@key"_s, key);
    this->raise_exception(exception);
}

void Env::raise_local_jump_error(Value exit_value, LocalJumpErrorType type, nat_int_t break_point) {
    auto message = new StringObject { type == LocalJumpErrorType::Return ? "unexpected return" : "break from proc-closure" };
    auto lje_class = find_top_level_const(this, "LocalJumpError"_s)->as_class();
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
    ExceptionObject *error = SystemCallError.send(this, "exception"_s, { Value::integer(errno) })->as_exception();
    raise_exception(error);
}

void Env::raise_errno(StringObject *detail) {
    auto SystemCallError = find_top_level_const(this, "SystemCallError"_s);
    ExceptionObject *error = SystemCallError.send(this, "exception"_s, { detail, Value::integer(errno) })->as_exception();
    raise_exception(error);
}

void Env::raise_no_method_error(Object *receiver, SymbolObject *name, MethodMissingReason reason) {
    String inspect_string;
    if (receiver->type() != Object::Type::Object) {
        inspect_string = String::format("{}:{}", receiver->inspect_str(this), receiver->klass()->inspect_str());
    } else {
        inspect_string = receiver->inspect_str(this);
    }
    String message;
    switch (reason) {
    case MethodMissingReason::Private:
        message = String::format("private method `{}' called for {}", name->string(), inspect_string);
        break;
    case MethodMissingReason::Protected:
        message = String::format("protected method `{}' called for {}", name->string(), inspect_string);
        break;
    case MethodMissingReason::Undefined:
        message = String::format("undefined method `{}' for {}", name->string(), inspect_string);
        break;
    default:
        NAT_UNREACHABLE();
    }
    auto NoMethodError = find_top_level_const(this, "NoMethodError"_s)->as_class();
    ExceptionObject *exception = new ExceptionObject { NoMethodError, new StringObject { std::move(message) } };
    exception->ivar_set(this, "@receiver"_s, receiver);
    exception->ivar_set(this, "@name"_s, name);
    this->raise_exception(exception);
}

void Env::raise_name_error(SymbolObject *name, String message) {
    auto NameError = find_top_level_const(this, "NameError"_s)->as_class();
    ExceptionObject *exception = new ExceptionObject { NameError, new StringObject { std::move(message) } };
    exception->ivar_set(this, "@name"_s, name);
    this->raise_exception(exception);
}

void Env::raise_name_error(StringObject *name, String message) {
    auto NameError = find_top_level_const(this, "NameError"_s)->as_class();
    ExceptionObject *exception = new ExceptionObject { NameError, new StringObject { std::move(message) } };
    exception->ivar_set(this, "@name"_s, name);
    this->raise_exception(exception);
}

void Env::raise_not_comparable_error(Value lhs, Value rhs) {
    String lhs_class = lhs->klass()->inspect_str();
    String rhs_inspect;
    if (rhs->is_integer() || rhs->is_float() || rhs->is_falsey()) {
        rhs_inspect = rhs->inspect_str(this);
    } else {
        rhs_inspect = rhs->klass()->inspect_str();
    }

    String message = String::format("comparison of {} with {} failed", lhs_class, rhs_inspect);
    raise("ArgumentError", std::move(message));
}

void Env::warn(String message) {
    Value _stderr = global_get("$stderr"_s);
    message = String::format("warning: {}", message);

    if (m_file && m_line) {
        message = String::format("{}:{}: {}", m_file, m_line, message);
    }

    _stderr.send(this, "puts"_s, { new StringObject { std::move(message) } });
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
        raise("ArgumentError", "missing keyword: {}", missing[0]->inspect_str(this));
    String message { "missing keywords: " };
    message.append(missing[0]->inspect_str(this));
    for (size_t i = 1; i < missing.size(); i++) {
        message.append(", ");
        message.append(missing[i]->inspect_str(this));
    }
    raise("ArgumentError", std::move(message));
}

void Env::ensure_no_extra_keywords(HashObject *kwargs) {
    if (!kwargs || kwargs->is_empty())
        return;
    auto it = kwargs->begin();
    if (kwargs->size() == 1)
        raise("ArgumentError", "unknown keyword: {}", it->key->inspect_str(this));
    String message { "unknown keywords: " };
    message.append(it->key->inspect_str(this));
    for (it++; it != kwargs->end(); it++) {
        message.append(", ");
        message.append(it->key->inspect_str(this));
    }
    raise("ArgumentError", std::move(message));
}

Value Env::last_match(bool to_s) {
    Env *env = non_block_env();
    if (env->m_match) {
        auto match = env->m_match;
        return to_s ? match->as_match_data()->to_s(this) : match;
    }
    return NilObject::the();
}

bool Env::has_last_match() {
    return non_block_env()->m_match;
}

void Env::set_last_match(MatchDataObject *match) {
    auto env = non_block_env();
    env->global_set("$~"_s, match);
    env->set_match(match);
    if (match) {
        env->global_set("$`"_s, match->pre_match(env));
        env->global_set("$'"_s, match->post_match(env));
    }
}

Value Env::exception_object() {
    auto e = exception();
    if (!e)
        return NilObject::the();
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

Value Env::var_get(const char *key, size_t index) {
    if (!m_vars || index >= m_vars->size())
        return NilObject::the();
    Value val = m_vars->at(index);
    if (val) {
        return val;
    } else {
        return NilObject::the();
    }
}

Value Env::var_set(const char *name, size_t index, bool allocate, Value val) {
    NAT_GC_GUARD_VALUE(val);

    size_t needed = index + 1;
    size_t current_size = m_vars ? m_vars->size() : 0;
    if (needed > current_size) {
        if (allocate) {
            if (!m_vars) {
                build_vars(needed);
            } else {
                m_vars->set_size(needed, NilObject::the());
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

void Env::visit_children(Visitor &visitor) {
    visitor.visit(m_vars);
    visitor.visit(m_outer);
    visitor.visit(m_block);
    visitor.visit(m_this_block);
    visitor.visit(m_caller);
    visitor.visit(m_method);
    visitor.visit(m_module);
    visitor.visit(m_match);
    visitor.visit(m_exception);
}
}
