#include <stdarg.h>

#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Env Env::new_block_env(Env *outer, Env *calling_env) {
    Env env { outer };
    env.block_env = true;
    env.caller = calling_env;
    return env;
}

Env Env::new_detatched_block_env(Env *outer) {
    Env env;
    env.global_env = outer->global_env;
    env.block_env = true;
    return env;
}

Value *Env::global_get(const char *name) {
    Env *env = this;
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a global variable name", name);
    }
    Value *val = static_cast<Value *>(hashmap_get(env->global_env->globals, name));
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

Value *Env::global_set(const char *name, Value *val) {
    Env *env = this;
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an global variable name", name);
    }
    hashmap_remove(env->global_env->globals, name);
    hashmap_put(env->global_env->globals, name, val);
    return val;
}

const char *Env::find_current_method_name() {
    Env *env = this;
    while ((!env->method_name || strcmp(env->method_name, "<block>") == 0) && env->outer) {
        env = env->outer;
    }
    if (strcmp(env->method_name, "<main>") == 0) return NULL;
    return env->method_name;
}

// note: returns a heap pointer that the caller must free
char *Env::build_code_location_name(Env *location_env) {
    do {
        if (location_env->method_name) {
            if (strcmp(location_env->method_name, "<block>") == 0) {
                if (location_env->outer) {
                    char *outer_name = build_code_location_name(location_env->outer);
                    char *name = strdup(StringValue::sprintf(this, "block in %s", outer_name)->c_str());
                    free(outer_name);
                    return name;
                } else {
                    return strdup("block");
                }
            } else {
                return strdup(location_env->method_name);
            }
        }
        location_env = location_env->outer;
    } while (location_env);
    return strdup("(unknown)");
}

Value *Env::raise(ClassValue *klass, const char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    StringValue *message = StringValue::vsprintf(this, message_format, args);
    va_end(args);
    ExceptionValue *exception = new ExceptionValue { this, klass, strdup(message->c_str()) };
    this->raise_exception(exception);
    return exception;
}

Value *Env::raise_exception(ExceptionValue *exception) {
    if (!exception->backtrace) { // only build a backtrace the first time the exception is raised (not on a re-raise)
        ArrayValue *bt = exception->backtrace = new ArrayValue { this };
        Env *bt_env = this;
        do {
            if (bt_env->file) {
                char *method_name = this->build_code_location_name(bt_env);
                bt->push(StringValue::sprintf(this, "%s:%d:in `%s'", bt_env->file, bt_env->line, method_name));
                free(method_name);
            }
            bt_env = bt_env->caller;
        } while (bt_env);
    }
    Env *env = this;
    while (env && !env->rescue) {
        env = env->caller;
    }
    assert(env);
    env->exception = exception;
    longjmp(env->jump_buf, 1);
}

Value *Env::raise_local_jump_error(Value *exit_value, const char *message) {
    Env *env = this;
    ExceptionValue *exception = new ExceptionValue { this, NAT_OBJECT->const_get(this, "LocalJumpError", true)->as_class(), message };
    exception->ivar_set(this, "@exit_value", exit_value);
    this->raise_exception(exception);
    return exception;
}

}
