#include "gc.h"
#include "natalie.h"

NatObject *nat_alloc(NatEnv *env) {
    NatObject *obj = nat_malloc(env, sizeof(NatObject));
    obj->flags = 0;
    obj->type = NAT_VALUE_OTHER;
    obj->included_modules_count = 0;
    obj->included_modules = NULL;
    obj->klass = NULL;
    obj->singleton_class = NULL;
    obj->constants.table = NULL;
    obj->ivars.table = NULL;
    obj->env.outer = NULL;
    int err = pthread_mutex_init(&obj->mutex, NULL);
    if (err) {
        fprintf(stderr, "Could not initialize mutex: %d\n", err);
        abort();
    }
    return obj;
}

void *nat_malloc(NatEnv *env, size_t size) {
    return malloc(size);
}

void *nat_calloc(NatEnv *env, size_t count, size_t size) {
    return calloc(count, size);
}

void *nat_realloc(NatEnv *env, void *ptr, size_t size) {
    return realloc(ptr, size);
}
