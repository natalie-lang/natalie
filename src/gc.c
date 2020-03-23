#include "gc.h"
#include "natalie.h"

void nat_gc_init(NatEnv *env, void *bottom_of_stack) {
    env->global_env->bottom_of_stack = bottom_of_stack;
}

NatHeapBlock *nat_gc_alloc_heap_block(NatGlobalEnv *global_env) {
    NatHeapBlock *block = calloc(1, sizeof(NatHeapBlock));
    block->next = NULL;
    NatObject *last = block->free_list = &block->storage[0];
    for (size_t i=1; i<NAT_HEAP_OBJECT_COUNT; i++) {
        last->next_free_object = &block->storage[i];
        last = &block->storage[i];
    }
    last->next_free_object = NULL;
    NatHeapBlock *next_block = global_env->heap;
    global_env->heap = block;
    block->next = next_block;
    if (&block->storage[NAT_HEAP_OBJECT_COUNT-1] > global_env->max_ptr) {
        global_env->max_ptr = &block->storage[NAT_HEAP_OBJECT_COUNT-1];
    }
    if (&block->storage[0] < global_env->min_ptr) {
        global_env->min_ptr = &block->storage[0];
    }
    return block;
}

NatObject *nat_gc_malloc(NatEnv *env) {
    NatHeapBlock *block = env->global_env->heap;
    NatObject *cell;
    do {
        cell = block->free_list;
        if (cell) {
            block->free_list = cell->next_free_object;
            return cell;
        }
        block = block->next;
    } while (block);
    nat_gc_alloc_heap_block(env->global_env);
    return nat_gc_malloc(env);
}

static void nat_gc_push_object(NatEnv *env, NatObject *objects, NatObject *obj) {
    if (obj && !NAT_IS_TAGGED_INT(obj)) {
        nat_array_push(env, objects, obj);
    }
}

static void nat_gc_gather_from_env(NatObject *objects, NatEnv *env) {
    for (size_t i=0; i<env->var_count; i++) {
        nat_gc_push_object(env, objects, env->vars[i]);
    }
    nat_gc_push_object(env, objects, env->exception);
    if (env->outer) {
        nat_gc_gather_from_env(objects, env->outer);
    }
}

NatObject *nat_gc_gather_roots(NatEnv *env) {
    NatGlobalEnv *global_env = env->global_env;
    void *dummy;
    void *top_of_stack = &dummy;
    NatObject *roots = nat_array(env);
    if (global_env->bottom_of_stack < top_of_stack) {
        fprintf(stderr, "Unsupported platform\n");
        abort();
    }
    NatObject *min_ptr = global_env->min_ptr;
    NatObject *max_ptr = global_env->max_ptr;
    for (void *p = global_env->bottom_of_stack; p >= top_of_stack; p--) {
        NatObject *ptr = *((NatObject**)p);
        if (ptr != roots && ptr >= min_ptr && ptr <= max_ptr) {
            nat_gc_push_object(env, roots, ptr);
        }
    }
    nat_gc_push_object(env, roots, global_env->Object);
    nat_gc_push_object(env, roots, global_env->Integer);
    nat_gc_push_object(env, roots, global_env->nil);
    nat_gc_push_object(env, roots, global_env->true_obj);
    nat_gc_push_object(env, roots, global_env->false_obj);
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(global_env->globals); iter; iter = hashmap_iter_next(global_env->globals, iter))
    {
        NatObject *obj = (NatObject *)hashmap_iter_get_data(iter);
        nat_gc_push_object(env, roots, obj);
    }
    nat_gc_gather_from_env(roots, env);
    return roots;
}

void nat_gc_unmark_all_objects(NatEnv *env) {
    NatHeapBlock *block = env->global_env->heap;
    do {
        for (size_t i=0; i<NAT_HEAP_OBJECT_COUNT; i++) {
            block->storage[i].marked = false;
        }
        block = block->next;
    } while (block);
}

NatObject *nat_gc_mark_live_objects(NatEnv *env) {
    NatObject *objects = nat_gc_gather_roots(env);

    for (size_t o=0; o<objects->ary_len; o++) {
        NatObject *obj = objects->ary[o];

        if (obj->marked) continue;
        obj->marked = true;

        nat_gc_push_object(env, objects, obj->klass);
        nat_gc_push_object(env, objects, obj->singleton_class);
        struct hashmap_iter *iter;
        for (iter = hashmap_iter(&obj->constants); iter; iter = hashmap_iter_next(&obj->constants, iter))
        {
            NatObject *obj = (NatObject *)hashmap_iter_get_data(iter);
            nat_gc_push_object(env, objects, obj);
        }
        for (iter = hashmap_iter(&obj->ivars); iter; iter = hashmap_iter_next(&obj->ivars, iter))
        {
            NatObject *obj = (NatObject *)hashmap_iter_get_data(iter);
            nat_gc_push_object(env, objects, obj);
        }

        nat_gc_gather_from_env(objects, &obj->env);

        NatHashKey *key;
        switch (obj->type) {
            case NAT_VALUE_ARRAY:
                for (size_t i=0; i<obj->ary_len; i++) {
                    nat_gc_push_object(env, objects, obj->ary[i]);
                }
                break;
            case NAT_VALUE_CLASS:
                nat_gc_push_object(env, objects, obj->superclass);
                for (size_t i=0; i<obj->included_modules_count; i++) {
                    nat_gc_push_object(env, objects, obj->included_modules[i]);
                }
                nat_gc_push_object(env, objects, obj->superclass);
                break;
            case NAT_VALUE_EXCEPTION:
                nat_gc_push_object(env, objects, obj->backtrace);
                break;
            case NAT_VALUE_FALSE:
                break;
            case NAT_VALUE_HASH:
                key = obj->key_list;
                do {
                    nat_gc_push_object(env, objects, key->key);
                    nat_gc_push_object(env, objects, key->val);
                    key = key->next;
                } while (key != obj->key_list);
                break;
            case NAT_VALUE_INTEGER:
                break;
            case NAT_VALUE_IO:
                break;
            case NAT_VALUE_MATCHDATA:
                break;
            case NAT_VALUE_MODULE:
                break;
            case NAT_VALUE_NIL:
                break;
            case NAT_VALUE_OTHER:
                break;
            case NAT_VALUE_PROC:
                break;
            case NAT_VALUE_RANGE:
                nat_gc_push_object(env, objects, obj->range_begin);
                nat_gc_push_object(env, objects, obj->range_end);
                break;
            case NAT_VALUE_REGEXP:
                break;
            case NAT_VALUE_STRING:
                break;
            case NAT_VALUE_SYMBOL:
                break;
            case NAT_VALUE_THREAD:
                nat_gc_push_object(env, objects, obj->thread_value);
                break;
            case NAT_VALUE_TRUE:
                break;
        }
    }
    return objects;
}

static void nat_gc_collect_dead_objects(NatEnv *env) {
    NatHeapBlock *block = env->global_env->heap;
    do {
        for (size_t i=0; i<NAT_HEAP_OBJECT_COUNT; i++) {
            NatObject *obj = &block->storage[i];
            if (obj->type && !obj->marked && NAT_TYPE(obj) != NAT_VALUE_SYMBOL) {
                // TODO: collect it!
            }
        }
        block = block->next;
    } while (block);
}

void nat_gc_collect(NatEnv *env) {
    nat_gc_unmark_all_objects(env);
    nat_gc_mark_live_objects(env);
    nat_gc_collect_dead_objects(env);
}

NatObject *nat_alloc(NatEnv *env, NatObject *klass, enum NatValueType type) {
    NatObject *obj = nat_gc_malloc(env);
    obj->klass = klass;
    obj->type = type;
    obj->flags = 0;
    obj->included_modules_count = 0;
    obj->included_modules = NULL;
    obj->singleton_class = NULL;
    obj->constants.table = NULL;
    obj->ivars.table = NULL;
    obj->env.global_env = NULL;
    int err = pthread_mutex_init(&obj->mutex, NULL);
    if (err) {
        fprintf(stderr, "Could not initialize mutex: %d\n", err);
        abort();
    }
    return obj;
}
