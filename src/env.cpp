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
                    char *name = heap_string(sprintf(this, "block in %s", outer_name)->str);
                    free(outer_name);
                    return name;
                } else {
                    return heap_string("block");
                }
            } else {
                return heap_string(location_env->method_name);
            }
        }
        location_env = location_env->outer;
    } while (location_env);
    return heap_string("(unknown)");
}

Value *Env::raise(ClassValue *klass, const char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    StringValue *message = vsprintf(this, message_format, args);
    va_end(args);
    ExceptionValue *exception = new ExceptionValue { this, klass, heap_string(message->str) };
    this->raise_exception(exception);
    return exception;
}

Value *Env::raise_exception(ExceptionValue *exception) {
    if (!exception->backtrace) { // only build a backtrace the first time the exception is raised (not on a re-raise)
        Value *bt = exception->backtrace = new ArrayValue { this };
        Env *bt_env = this;
        do {
            if (bt_env->file) {
                char *method_name = this->build_code_location_name(bt_env);
                array_push(this, bt, sprintf(this, "%s:%d:in `%s'", bt_env->file, bt_env->line, method_name));
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
    ExceptionValue *exception = new ExceptionValue { this, const_get(this, NAT_OBJECT, "LocalJumpError", true)->as_class(), message };
    exception->ivar_set(this, "@exit_value", exit_value);
    this->raise_exception(exception);
    return exception;
}

}
