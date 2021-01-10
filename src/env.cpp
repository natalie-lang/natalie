#include <stdarg.h>

#include "natalie.hpp"

namespace Natalie {

Env Env::new_detatched_env(Env *outer) {
    return Env { outer->global_env() };
}

void Env::build_vars(size_t size) {
    m_vars = new Vector<ValuePtr> { size, static_cast<ValuePtr>(nil_obj()) };
}

ValuePtr Env::global_get(const char *name) {
    Env *env = this;
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        env->raise("NameError", "`%s' is not allowed as a global variable name", name);
    }
    ValuePtr val = static_cast<ValuePtr>(hashmap_get(env->global_env()->globals(), name));
    if (val) {
        return val;
    } else {
        return env->nil_obj();
    }
}

ValuePtr Env::global_set(const char *name, ValuePtr val) {
    Env *env = this;
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        env->raise("NameError", "`%s' is not allowed as an global variable name", name);
    }
    hashmap_remove(env->global_env()->globals(), name);
    hashmap_put(env->global_env()->globals(), name, val);
    return val;
}

const char *Env::find_current_method_name() {
    Env *env = this;
    while ((!env->method_name() || strcmp(env->method_name(), "<block>") == 0) && env->outer()) {
        env = env->outer();
    }
    if (strcmp(env->method_name(), "<main>") == 0) return nullptr;
    return env->method_name();
}

// note: returns a heap pointer that the caller must free
char *Env::build_code_location_name(Env *location_env) {
    do {
        if (location_env->method_name()) {
            if (strcmp(location_env->method_name(), "<block>") == 0) {
                if (location_env->outer()) {
                    char *outer_name = build_code_location_name(location_env->outer());
                    char *name = GC_STRDUP(StringValue::sprintf(this, "block in %s", outer_name)->c_str());
                    GC_FREE(outer_name);
                    return name;
                } else {
                    return GC_STRDUP("block");
                }
            } else {
                return GC_STRDUP(location_env->method_name());
            }
        }
        location_env = location_env->outer();
    } while (location_env);
    return GC_STRDUP("(unknown)");
}

void Env::raise(ClassValue *klass, StringValue *message) {
    ExceptionValue *exception = new ExceptionValue { this, klass, message->c_str() };
    this->raise_exception(exception);
}

void Env::raise(ClassValue *klass, const char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    StringValue *message = StringValue::vsprintf(this, message_format, args);
    raise(klass, message);
}

void Env::raise(const char *class_name, const char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    StringValue *message = StringValue::vsprintf(this, message_format, args);
    va_end(args);
    ClassValue *klass = Object()->const_fetch(class_name)->as_class();
    ExceptionValue *exception = new ExceptionValue { this, klass, message->c_str() };
    this->raise_exception(exception);
}

void Env::raise_exception(ExceptionValue *exception) {
    if (!exception->backtrace()) {
        // only build a backtrace the first time the exception is raised (not on a re-raise)
        exception->build_backtrace(this);
    }
    throw exception;
}

void Env::raise_local_jump_error(ValuePtr exit_value, const char *message) {
    Env *env = this;
    ExceptionValue *exception = new ExceptionValue { this, env->Object()->const_find(this, "LocalJumpError")->as_class(), message };
    exception->ivar_set(this, "@exit_value", exit_value);
    this->raise_exception(exception);
}

void Env::raise_errno() {
    ValuePtr exception_args[] = { new IntegerValue { this, errno } };
    ExceptionValue *error = Object()->const_find(this, "SystemCallError")->send(this, "exception", 1, exception_args, nullptr)->as_exception();
    raise_exception(error);
}

void Env::assert_argc(size_t argc, size_t expected) {
    if (argc != expected) {
        raise("ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, expected);
    }
}

void Env::assert_argc(size_t argc, size_t expected_low, size_t expected_high) {
    if (argc < expected_low || argc > expected_high) {
        raise("ArgumentError", "wrong number of arguments (given %d, expected %d..%d)", argc, expected_low, expected_high);
    }
}

void Env::assert_argc_at_least(size_t argc, size_t expected) {
    if (argc < expected) {
        raise("ArgumentError", "wrong number of arguments (given %d, expected %d+)", argc, expected);
    }
}

void Env::assert_block_given(Block *block) {
    if (!block) {
        raise("ArgumentError", "called without a block");
    }
}

ValuePtr Env::last_match() {
    if (m_match) {
        return m_match;
    } else {
        return nil_obj();
    }
}

ValuePtr Env::var_get(const char *key, size_t index) {
    if (index >= m_vars->size()) {
        printf("Trying to get variable `%s' at index %zu which is not set.\n", key, index);
        abort();
    }
    ValuePtr val = (*m_vars)[index];
    if (val) {
        return val;
    } else {
        Env *env = this;
        return env->nil_obj();
    }
}

ValuePtr Env::var_set(const char *name, size_t index, bool allocate, ValuePtr val) {
    size_t needed = index + 1;
    size_t current_size = m_vars ? m_vars->size() : 0;
    if (needed > current_size) {
        if (allocate) {
            if (!m_vars) {
                m_vars = new Vector<ValuePtr> { needed, nil_obj() };
            } else {
                m_vars->set_size(needed, nil_obj());
            }
        } else {
            printf("Tried to set a variable without first allocating space for it.\n");
            abort();
        }
    }
    (*m_vars)[index] = val;
    return val;
}

}
