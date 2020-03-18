#include "gc.h"
#include "natalie.h"

NatObject *nat_alloc(NatEnv *env) {
    NatObject *obj = malloc(sizeof(NatObject));
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

