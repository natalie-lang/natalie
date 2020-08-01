#include <stdarg.h>

#include "natalie.hpp"

namespace Natalie {

Env Env::new_block_env(Env *outer, Env *calling_env) {
    Env env { outer };
    env.set_block_env();
    env.set_caller(calling_env);
    return env;
}

Env Env::new_detatched_block_env(Env *outer) {
    Env env;
    env.set_global_env(outer->global_env());
    env.set_block_env();
    return env;
}

Value *Env::global_get(const char *name) {
    Env *env = this;
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a global variable name", name);
    }
    Value *val = static_cast<Value *>(hashmap_get(env->global_env()->globals(), name));
    if (val) {
        return val;
    } else {
        return env->nil_obj();
    }
}

Value *Env::global_set(const char *name, Value *val) {
    Env *env = this;
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an global variable name", name);
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
                    char *name = strdup(StringValue::sprintf(this, "block in %s", outer_name)->c_str());
                    free(outer_name);
                    return name;
                } else {
                    return strdup("block");
                }
            } else {
                return strdup(location_env->method_name());
            }
        }
        location_env = location_env->outer();
    } while (location_env);
    return strdup("(unknown)");
}

Value *Env::raise(ClassValue *klass, const char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    StringValue *message = StringValue::vsprintf(this, message_format, args);
    va_end(args);
    ExceptionValue *exception = new ExceptionValue { this, klass, message->c_str() };
    this->raise_exception(exception);
    return exception;
}

Value *Env::raise_exception(ExceptionValue *exception) {
    if (!exception->backtrace()) {
        // only build a backtrace the first time the exception is raised (not on a re-raise)
        exception->build_backtrace(this);
    }
    throw exception;
}

Value *Env::raise_local_jump_error(Value *exit_value, const char *message) {
    Env *env = this;
    ExceptionValue *exception = new ExceptionValue { this, env->Object()->const_get(this, "LocalJumpError", true)->as_class(), message };
    exception->ivar_set(this, "@exit_value", exit_value);
    this->raise_exception(exception);
    return exception;
}

Value *Env::last_match() {
    if (m_match) {
        return m_match;
    } else {
        return nil_obj();
    }
}

Value *Env::var_get(const char *key, ssize_t index) {
    if (index >= m_vars->size()) {
        printf("Trying to get variable `%s' at index %zu which is not set.\n", key, index);
        abort();
    }
    Value *val = (*m_vars)[index];
    if (val) {
        return val;
    } else {
        Env *env = this;
        return env->nil_obj();
    }
}

Value *Env::var_set(const char *name, ssize_t index, bool allocate, Value *val) {
    ssize_t needed = index + 1;
    ssize_t current_size = m_vars ? m_vars->size() : 0;
    if (needed > current_size) {
        if (allocate) {
            if (!m_vars) {
                m_vars = new Vector<Value *> { needed };
            } else {
                m_vars->set_size(needed);
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
