#include "gc.h"
#include "natalie.h"

NatObject *nat_alloc(NatEnv *env, NatObject *klass, enum NatValueType type) {
    NatObject *obj = malloc(sizeof(NatObject));
    obj->klass = klass;
    obj->type = type;
    obj->flags = 0;
    obj->included_modules_count = 0;
    obj->included_modules = NULL;
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
