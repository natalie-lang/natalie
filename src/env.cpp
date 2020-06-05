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

}
