#include <stdarg.h>

#include "natalie.hpp"
#include "natalie/string.hpp"

namespace Natalie {

using namespace TM;

void Env::build_vars(size_t size) {
    m_vars = new ManagedVector<Value>(size, NilObject::the());
}

Value Env::global_get(SymbolObject *name) {
    return GlobalEnv::the()->global_get(this, name);
}

Value Env::global_set(SymbolObject *name, Value val) {
    return GlobalEnv::the()->global_set(this, name, val);
}

Method *Env::current_method() {
    Env *env = this;
    while (!env->method() && env->outer()) {
        env = env->outer();
    }
    return env->method();
}

const String *Env::build_code_location_name(Env *location_env) {
    if (location_env->is_main())
        return new String("<main>");
    if (location_env->method())
        return new String(location_env->method()->name());
    // we're in a block, so try to build a string like "block in foo", "block in block in foo", etc.
    if (location_env->outer()) {
        auto outer_name = build_code_location_name(location_env->outer());
        return String::format("block in {}", outer_name);
    }
    // fall back to just "block" if we don't know where this block came from
    return new String("block");
}

void Env::raise(ClassObject *klass, StringObject *message) {
    ExceptionObject *exception = new ExceptionObject { klass, message };
    this->raise_exception(exception);
}

void Env::raise(ClassObject *klass, const String *message) {
    ExceptionObject *exception = new ExceptionObject { klass, new StringObject { *message } };
    this->raise_exception(exception);
}

void Env::raise(const char *class_name, const String *message) {
    ClassObject *klass = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern(class_name))->as_class();
    ExceptionObject *exception = new ExceptionObject { klass, new StringObject { *message } };
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

void Env::raise_local_jump_error(Value exit_value, LocalJumpErrorType type) {
    auto message = new StringObject { type == LocalJumpErrorType::Return ? "unexpected return" : "break from proc-closure" };
    auto lje_class = find_top_level_const(this, "LocalJumpError"_s)->as_class();
    ExceptionObject *exception = new ExceptionObject { lje_class, message };
    exception->set_local_jump_error_type(type);
    exception->ivar_set(this, "@exit_value"_s, exit_value);
    if (type == LocalJumpErrorType::Break) {
        assert(m_this_block);
        exception->set_local_jump_error_env(m_this_block->calling_env());
    } else {
        exception->set_local_jump_error_env(this);
    }
    this->raise_exception(exception);
}

void Env::raise_errno() {
    auto SystemCallError = find_top_level_const(this, "SystemCallError"_s);
    ExceptionObject *error = SystemCallError.send(this, "exception"_s, { Value::integer(errno) })->as_exception();
    raise_exception(error);
}

void Env::raise_name_error(SymbolObject *name, const String *message) {
    auto NameError = find_top_level_const(this, "NameError"_s)->as_class();
    ExceptionObject *exception = new ExceptionObject { NameError, new StringObject { *message } };
    exception->ivar_set(this, "@name"_s, name);
    this->raise_exception(exception);
}

void Env::warn(const String *message) {
    Value _stderr = global_get("$stderr"_s);
    message = String::format("{}:{}: warning: {}", m_file, m_line, message);
    _stderr.send(this, "puts"_s, { new StringObject { message } });
}

void Env::ensure_argc_is(size_t argc, size_t expected) {
    if (argc != expected) {
        raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, expected);
    }
}

void Env::ensure_argc_between(size_t argc, size_t expected_low, size_t expected_high) {
    if (argc < expected_low || argc > expected_high) {
        raise("ArgumentError", "wrong number of arguments (given {}, expected {}..{})", argc, expected_low, expected_high);
    }
}

void Env::ensure_argc_at_least(size_t argc, size_t expected) {
    if (argc < expected) {
        raise("ArgumentError", "wrong number of arguments (given {}, expected {}+)", argc, expected);
    }
}

void Env::ensure_block_given(Block *block) {
    if (!block) {
        raise("ArgumentError", "called without a block");
    }
}

Value Env::last_match() {
    Env *env = non_block_env();
    if (env->m_match)
        return env->m_match;
    return NilObject::the();
}

bool Env::has_last_match() {
    return non_block_env()->m_match;
}

void Env::set_last_match(Value match) {
    non_block_env()->global_set("$~"_s, match);
    non_block_env()->set_match(match);
}

Value Env::var_get(const char *key, size_t index) {
    if (index >= m_vars->size())
        return NilObject::the();
    Value val = m_vars->at(index);
    if (val) {
        return val;
    } else {
        return NilObject::the();
    }
}

Value Env::var_set(const char *name, size_t index, bool allocate, Value val) {
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
    visitor.visit(m_match);
}
}
