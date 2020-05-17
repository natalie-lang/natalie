#include "gc.h"
#include "natalie.h"

void nat_gc_init(NatEnv *env, void *bottom_of_stack) {
    env->global_env->bottom_of_stack = bottom_of_stack;
    env->global_env->gc_enabled = true;
}

NatHeapBlock *nat_gc_alloc_heap_block(NatGlobalEnv *global_env) {
    NatHeapBlock *block = calloc(1, NAT_HEAP_BLOCK_SIZE);
    NatHeapCell *cell = NAT_HEAP_BLOCK_FIRST_CELL(block);
    cell->size = NAT_HEAP_BLOCK_SIZE - NAT_HEAP_BLOCK_HEADER_SIZE - NAT_HEAP_CELL_HEADER_SIZE;
    hashmap_put(global_env->heap_cells, cell, block);
    block->free_list = cell;
    NAT_LIST_PREPEND(global_env->heap, block);
    NatObject *max = (NatObject*)((char*)block + NAT_HEAP_BLOCK_SIZE);
    if (max > global_env->max_ptr) {
        global_env->max_ptr = max;
    }
    NatObject *min = (NatObject*)((char*)block + NAT_HEAP_BLOCK_HEADER_SIZE);
    if (min + NAT_HEAP_CELL_HEADER_SIZE < global_env->min_ptr) {
        global_env->min_ptr = min;
    }
    ssize_t usable = NAT_HEAP_BLOCK_SIZE - NAT_HEAP_BLOCK_HEADER_SIZE - NAT_HEAP_CELL_HEADER_SIZE;
    global_env->bytes_available += usable;
    global_env->bytes_total += usable;
    return block;
}

NatObject *nat_gc_malloc(NatEnv *env) {
    ssize_t size = sizeof(NatObject);
    NatHeapBlock *block = env->global_env->heap;
    do {
        NatHeapCell *cell = block->free_list;
        NatHeapCell *prev_cell = NULL;
        while (cell) {
            if (cell && cell->size >= size) {
                ssize_t remaining = cell->size - size - NAT_HEAP_CELL_HEADER_SIZE;
                if (remaining > NAT_HEAP_MIN_CELL_SIZE) {
                    NatHeapCell *new_cell = (NatHeapCell*)((char*)cell + NAT_HEAP_CELL_HEADER_SIZE + size);
                    new_cell->next = cell->next;
                    new_cell->size = remaining;
                    hashmap_put(env->global_env->heap_cells, new_cell, block);
                    cell->size = size;
                    if (prev_cell) {
                        prev_cell->next = new_cell;
                    } else {
                        block->free_list = new_cell;
                    }
                    env->global_env->bytes_available -= (size + NAT_HEAP_CELL_HEADER_SIZE);
                } else {
                    if (prev_cell) {
                        prev_cell->next = cell->next;
                    } else {
                        block->free_list = cell->next;
                    }
                    env->global_env->bytes_available -= cell->size;
                }
                NAT_LIST_PREPEND(block->used_list, cell);
                return NAT_HEAP_CELL_START_USABLE(cell);
            } else {
            }
            prev_cell = cell;
            cell = cell->next;
        }
        block = block->next;
    } while (block);
    nat_gc_alloc_heap_block(env->global_env);
    return nat_gc_malloc(env);
}

static void nat_gc_push_object(NatEnv *env, NatObject *objects, NatObject *obj) {
    if (obj && !NAT_IS_TAGGED_INT(obj)) {

        // TODO: remove these checks later :-)
        if (obj < env->global_env->min_ptr || obj > env->global_env->max_ptr) {
            printf("GC fatal: memory corruption (obj %p outside of heap)\n", obj);
            abort();
        }
        if (obj->klass < env->global_env->min_ptr || obj->klass > env->global_env->max_ptr) {
            printf("GC fatal: memory corruption (obj %p klass outside of heap)\n", obj);
            abort();
        }

        nat_array_push(env, objects, obj);
    }
}

static void nat_gc_gather_from_env(NatObject *objects, NatEnv *env) {
    assert(env->var_count < 10000); // just a sanity check :-)
    for (ssize_t i = 0; i < env->var_count; i++) {
        nat_gc_push_object(env, objects, env->vars[i]);
    }
    if (env->exception) nat_gc_push_object(env, objects, env->exception);
    if (env->outer) {
        nat_gc_gather_from_env(objects, env->outer);
    }
    if (env->caller) {
        nat_gc_gather_from_env(objects, env->caller);
    }
}

bool nat_gc_is_heap_ptr(NatEnv *env, NatObject *ptr) {
    if (ptr < env->global_env->min_ptr || ptr > env->global_env->max_ptr) {
        return false;
    }
    NatHeapCell *cell = NAT_HEAP_CELL_FROM_OBJ(ptr);
    if (hashmap_get(env->global_env->heap_cells, cell)) {
        if (ptr->type) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

NatObject *nat_gc_gather_roots(NatEnv *env) {
    NatGlobalEnv *global_env = env->global_env;
    jmp_buf jump_buf;
    setjmp(jump_buf);
    void *dummy;
    void *top_of_stack = &dummy;
    NatObject *roots = nat_array(env);
    if (global_env->bottom_of_stack < top_of_stack) {
        fprintf(stderr, "Unsupported platform\n");
        abort();
    }
    for (void *p = global_env->bottom_of_stack; p >= top_of_stack; p -= 4) {
        NatObject *ptr = *((NatObject **)p);
        if (nat_gc_is_heap_ptr(env, ptr)) {
            nat_gc_push_object(env, roots, ptr);
        }
    }
    nat_gc_push_object(env, roots, global_env->Object);
    nat_gc_push_object(env, roots, global_env->Integer);
    nat_gc_push_object(env, roots, global_env->nil);
    nat_gc_push_object(env, roots, global_env->true_obj);
    nat_gc_push_object(env, roots, global_env->false_obj);
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(global_env->globals); iter; iter = hashmap_iter_next(global_env->globals, iter)) {
        NatObject *obj = (NatObject *)hashmap_iter_get_data(iter);
        nat_gc_push_object(env, roots, obj);
    }
    nat_gc_gather_from_env(roots, env);
    return roots;
}

void nat_gc_unmark_all_objects(NatEnv *env) {
    NatHeapBlock *block = env->global_env->heap;
    do {
        NatHeapCell *cell = block->used_list;
        while (cell) {
            cell->mark = false;
            cell = cell->next;
        };
        block = block->next;
    } while (block);
}

NatObject *nat_gc_mark_live_objects(NatEnv *env) {
    NatObject *objects = nat_gc_gather_roots(env);

    for (ssize_t o = 0; o < objects->ary_len; o++) {
        NatObject *obj = objects->ary[o];
        if (obj == objects) continue;

        NatHeapCell *cell = NAT_HEAP_CELL_FROM_OBJ(obj);
        if (cell->mark) continue;

        cell->mark = true;

        nat_gc_push_object(env, objects, NAT_OBJ_CLASS(obj));
        nat_gc_push_object(env, objects, obj->owner);
        if (obj->singleton_class) nat_gc_push_object(env, objects, obj->singleton_class);
        struct hashmap_iter *iter;
        if (obj->constants.table) {
            for (iter = hashmap_iter(&obj->constants); iter; iter = hashmap_iter_next(&obj->constants, iter)) {
                NatObject *o = (NatObject *)hashmap_iter_get_data(iter);
                nat_gc_push_object(env, objects, o);
            }
        }
        if (obj->ivars.table) {
            for (iter = hashmap_iter(&obj->ivars); iter; iter = hashmap_iter_next(&obj->ivars, iter)) {
                NatObject *o = (NatObject *)hashmap_iter_get_data(iter);
                nat_gc_push_object(env, objects, o);
            }
        }
        if (obj->cvars.table) {
            for (iter = hashmap_iter(&obj->cvars); iter; iter = hashmap_iter_next(&obj->cvars, iter)) {
                NatObject *o = (NatObject *)hashmap_iter_get_data(iter);
                nat_gc_push_object(env, objects, o);
            }
        }

        if (NAT_OBJ_HAS_ENV(obj)) {
            assert(!obj->env.caller); // NatEnv caller should never exist on an object!
            nat_gc_gather_from_env(objects, &obj->env);
        }

        NatHashKey *key;
        switch (obj->type) {
        case NAT_VALUE_ARRAY:
            for (ssize_t i = 0; i < obj->ary_len; i++) {
                nat_gc_push_object(env, objects, obj->ary[i]);
            }
            break;
        case NAT_VALUE_CLASS:
            nat_gc_push_object(env, objects, obj->superclass);
            for (ssize_t i = 0; i < obj->included_modules_count; i++) {
                nat_gc_push_object(env, objects, obj->included_modules[i]);
            }
            if (obj->methods.table) {
                for (iter = hashmap_iter(&obj->methods); iter; iter = hashmap_iter_next(&obj->methods, iter)) {
                    NatMethod *method = (NatMethod *)hashmap_iter_get_data(iter);
                    if (NAT_OBJ_HAS_ENV(method)) nat_gc_gather_from_env(objects, &method->env);
                }
            }
            break;
        case NAT_VALUE_ENCODING:
            nat_gc_push_object(env, objects, obj->encoding_names);
            break;
        case NAT_VALUE_EXCEPTION:
            nat_gc_push_object(env, objects, obj->backtrace);
            break;
        case NAT_VALUE_FALSE:
            break;
        case NAT_VALUE_HASH:
            if (obj->key_list) {
                key = obj->key_list;
                do {
                    nat_gc_push_object(env, objects, key->key);
                    nat_gc_push_object(env, objects, key->val);
                    key = key->next;
                } while (key != obj->key_list);
            }
            nat_gc_push_object(env, objects, obj->hash_default_value);
            if (obj->hash_default_block && NAT_OBJ_HAS_ENV(obj->hash_default_block)) nat_gc_gather_from_env(objects, &obj->hash_default_block->env);
            break;
        case NAT_VALUE_INTEGER:
            break;
        case NAT_VALUE_IO:
            break;
        case NAT_VALUE_MATCHDATA:
            break;
        case NAT_VALUE_MODULE:
            if (obj->methods.table) {
                for (iter = hashmap_iter(&obj->methods); iter; iter = hashmap_iter_next(&obj->methods, iter)) {
                    NatMethod *method = (NatMethod *)hashmap_iter_get_data(iter);
                    if (NAT_OBJ_HAS_ENV(method)) nat_gc_gather_from_env(objects, &method->env);
                }
            }
            break;
        case NAT_VALUE_NIL:
            break;
        case NAT_VALUE_OTHER:
            break;
        case NAT_VALUE_PROC:
            nat_gc_gather_from_env(objects, &obj->block->env);
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
        case NAT_VALUE_TRUE:
            break;
        case NAT_VALUE_VOIDP:
            break;
        }
    }
    return objects;
}

static void nat_destroy_hash_key_list(NatObject *obj) {
    if (obj->key_list) {
        NatHashKey *key = obj->key_list;
        do {
            NatHashKey *next_key = key->next;
            free(key);
            key = next_key;
        } while (key != obj->key_list);
    }
}

static void nat_free_object(NatEnv *env, NatObject *obj) {
    if (obj->constants.table) hashmap_destroy(&obj->constants);
    if (obj->ivars.table) hashmap_destroy(&obj->ivars);
    struct hashmap_iter *iter;
    switch (obj->type) {
    case NAT_VALUE_ARRAY:
        free(obj->ary);
        break;
    case NAT_VALUE_CLASS:
        free(obj->class_name);
        if (obj->methods.table) {
            for (iter = hashmap_iter(&obj->methods); iter; iter = hashmap_iter_next(&obj->methods, iter)) {
                NatMethod *method = (NatMethod *)hashmap_iter_get_data(iter);
                free(method);
            }
            hashmap_destroy(&obj->methods);
        }
        if (obj->cvars.table) hashmap_destroy(&obj->cvars);
        free(obj->included_modules);
        break;
    case NAT_VALUE_ENCODING:
        break;
    case NAT_VALUE_EXCEPTION:
        free(obj->message);
        break;
    case NAT_VALUE_FALSE:
        break;
    case NAT_VALUE_HASH:
        nat_destroy_hash_key_list(obj);
        for (iter = hashmap_iter(&obj->hashmap); iter; iter = hashmap_iter_next(&obj->hashmap, iter)) {
            free((NatHashVal *)hashmap_iter_get_data(iter));
        }
        hashmap_destroy(&obj->hashmap);
        free(obj->hash_default_block);
        break;
    case NAT_VALUE_INTEGER:
        break;
    case NAT_VALUE_IO:
        break;
    case NAT_VALUE_MATCHDATA:
        onig_region_free(obj->matchdata_region, true);
        free(obj->matchdata_str);
        break;
    case NAT_VALUE_MODULE:
        free(obj->class_name);
        if (obj->methods.table) {
            for (iter = hashmap_iter(&obj->methods); iter; iter = hashmap_iter_next(&obj->methods, iter)) {
                NatMethod *method = (NatMethod *)hashmap_iter_get_data(iter);
                free(method);
            }
            hashmap_destroy(&obj->methods);
        }
        if (obj->cvars.table) hashmap_destroy(&obj->cvars);
        free(obj->included_modules);
        break;
    case NAT_VALUE_NIL:
        break;
    case NAT_VALUE_OTHER:
        break;
    case NAT_VALUE_PROC:
        free(obj->block);
        break;
    case NAT_VALUE_RANGE:
        break;
    case NAT_VALUE_REGEXP:
        onig_free(obj->regexp);
        free(obj->regexp_str);
        break;
    case NAT_VALUE_STRING:
        free(obj->str);
        break;
    case NAT_VALUE_SYMBOL:
        free(obj->symbol);
        break;
    case NAT_VALUE_TRUE:
        break;
    case NAT_VALUE_VOIDP:
        break;
    }
    obj->type = NAT_VALUE_NIL;
    obj->klass = NAT_NIL->klass;
}

static NatHeapCell *find_sibling_heap_cell(NatHeapCell *first_cell, NatHeapCell *cell) {
    NatHeapCell *sibling_cell = first_cell;
    while (sibling_cell) {
        if (sibling_cell->next == cell) {
            return sibling_cell;
        }
        sibling_cell = sibling_cell->next;
    }
    return NULL;
}

static void nat_gc_collect_object(NatEnv *env, NatHeapBlock *block, NatObject *obj) {
    NatHeapCell *cell = NAT_HEAP_CELL_FROM_OBJ(obj);

    nat_free_object(env, obj);
    memset(obj, 0, cell->size);

    if (cell == block->used_list) { // first cell in list
        block->used_list = cell->next;
    } else {
        NatHeapCell *sibling_cell = find_sibling_heap_cell(block->used_list, cell);
        assert(sibling_cell);
        sibling_cell->next = cell->next;
    }

    NAT_LIST_PREPEND(block->free_list, cell); // TODO: merge with adjacent cells if possible

    env->global_env->bytes_available += cell->size;
}

static void nat_gc_collect_dead_objects(NatEnv *env) {
    NatHeapBlock *block = env->global_env->heap;
    while (block) {
        NatHeapCell *cell = block->used_list;
        while (cell) {
            NatHeapCell *next_cell = cell->next;
            if (!cell->mark) {
                NatObject *obj = NAT_HEAP_OBJ_FROM_CELL(cell);
                if (NAT_TYPE(obj) != NAT_VALUE_SYMBOL) {
                    nat_gc_collect_object(env, block, obj);
                }
            }
            cell = next_cell;
        }
        block = block->next;
    }
}

void nat_gc_collect(NatEnv *env) {
#ifdef NAT_GC_DISABLE
    return;
#endif
    if (!env->global_env->gc_enabled) return;
    env->global_env->gc_enabled = false;
    nat_gc_unmark_all_objects(env);
    nat_gc_mark_live_objects(env);
    nat_gc_collect_dead_objects(env);
    while (nat_gc_bytes_available_ratio(env) < NAT_HEAP_MIN_AVAIL_AFTER_COLLECTION_RATIO) {
        // still not enough, even after a collection
        nat_gc_alloc_heap_block(env->global_env);
    }
    env->global_env->gc_enabled = true;
}

void nat_gc_collect_all(NatEnv *env) {
    nat_gc_unmark_all_objects(env);
    nat_gc_collect_dead_objects(env);
}

double nat_gc_bytes_available_ratio(NatEnv *env) {
    return (double)env->global_env->bytes_available / (double)env->global_env->bytes_total;
}

NatObject *nat_alloc(NatEnv *env, NatObject *klass, enum NatValueType type) {
#ifdef NAT_GC_COLLECT_DEBUG
    nat_gc_collect(env);
#else
    if (nat_gc_bytes_available_ratio(env) < NAT_HEAP_MIN_AVAIL_RATIO) {
        nat_gc_collect(env);
    }
#endif
    NatObject *obj = nat_gc_malloc(env);
    memset(obj, 0, sizeof(NatObject));
    obj->klass = klass;
    obj->type = type;
    return obj;
}
