#include "gc.hpp"
#include "natalie.hpp"

namespace Natalie {

void *gc_abort_if_collected = NULL;

static void push_cell(Env *env, Vector *cells, HeapCell *cell) {
    if (!cell) return;

    // TODO: remove this check later :-)
    if (cell < env->global_env->min_ptr || cell > env->global_env->max_ptr) {
        printf("GC fatal: memory corruption (cell %p outside of heap)\n", cell);
        abort();
    }

    vector_push(cells, cell);
}

// TODO: remove this check later :-)
static void verify_env_is_valid(Env *env) {
    Env *e = env;
    int depth = 0;
    while (e->caller) {
        depth++;
        e = e->caller;
    }
    if (e != env->global_env->top_env) {
        printf("env->caller at depth %i chain not valid!\n", depth);
        abort();
    }
}

static void push_object(Env *env, Vector *cells, Value *obj) {
    if (!obj) return;
    if (NAT_IS_TAGGED_INT(obj)) return;
    push_cell(env, cells, NAT_HEAP_CELL_FROM_OBJ(obj));
}

static void gather_from_env(Vector *cells, Env *env) {
    for (ssize_t i = 0; i < vector_size(env->vars); i++) {
        push_object(env, cells, static_cast<Value *>(vector_get(env->vars, i)));
    }
    push_object(env, cells, env->exception);
    if (env->outer) {
        gather_from_env(cells, env->outer);
    }
    if (env->caller) {
        gather_from_env(cells, env->caller);
    }
}

static bool is_heap_ptr(Env *env, Value *ptr) {
    if ((HeapCell *)ptr < env->global_env->min_ptr || (HeapCell *)ptr > env->global_env->max_ptr) {
        return false;
    }
    HeapCell *cell = NAT_HEAP_CELL_FROM_OBJ(ptr);
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

static Vector *gather_roots(Env *env) {
    GlobalEnv *global_env = env->global_env;
    jmp_buf jump_buf;
    setjmp(jump_buf);
    void *dummy;
    void *top_of_stack = &dummy;
    Vector *roots = vector(10);
    if (global_env->bottom_of_stack < top_of_stack) {
        fprintf(stderr, "Unsupported platform\n");
        abort();
    }
    for (char *p = static_cast<char *>(global_env->bottom_of_stack); p >= top_of_stack; p -= 4) {
        Value *ptr = *((Value **)p);
        if (is_heap_ptr(env, ptr)) {
            push_cell(env, roots, NAT_HEAP_CELL_FROM_OBJ(ptr));
        }
    }
    push_object(env, roots, global_env->Object);
    push_object(env, roots, global_env->Integer);
    push_object(env, roots, global_env->nil);
    push_object(env, roots, global_env->true_obj);
    push_object(env, roots, global_env->false_obj);
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(global_env->globals); iter; iter = hashmap_iter_next(global_env->globals, iter)) {
        Value *obj = (Value *)hashmap_iter_get_data(iter);
        push_object(env, roots, obj);
    }
    verify_env_is_valid(env);
    gather_from_env(roots, env);
    return roots;
}

static void unmark_all_cells(Env *env) {
    HeapBlock *block = env->global_env->heap;
    do {
        HeapCell *cell = block->used_list;
        while (cell) {
            cell->mark = false;
            cell = cell->next;
        };
        block = block->next;
    } while (block);
}

static void trace_object(Env *env, Vector *cells, Value *obj) {
    push_object(env, cells, NAT_OBJ_CLASS(obj));
    push_object(env, cells, obj->owner);
    push_object(env, cells, obj->singleton_class);
    struct hashmap_iter *iter;
    if (obj->constants.table) {
        for (iter = hashmap_iter(&obj->constants); iter; iter = hashmap_iter_next(&obj->constants, iter)) {
            Value *o = (Value *)hashmap_iter_get_data(iter);
            push_object(env, cells, o);
        }
    }
    if (obj->ivars.table) {
        for (iter = hashmap_iter(&obj->ivars); iter; iter = hashmap_iter_next(&obj->ivars, iter)) {
            Value *o = (Value *)hashmap_iter_get_data(iter);
            push_object(env, cells, o);
        }
    }
    if (obj->cvars.table) {
        for (iter = hashmap_iter(&obj->cvars); iter; iter = hashmap_iter_next(&obj->cvars, iter)) {
            Value *o = (Value *)hashmap_iter_get_data(iter);
            push_object(env, cells, o);
        }
    }

    if (NAT_OBJ_HAS_ENV(obj)) {
        assert(!obj->env.caller); // Env caller should never exist on an object!
        gather_from_env(cells, &obj->env);
    }

    HashKey *key;
    switch (obj->type) {
    case NAT_VALUE_ARRAY:
        for (ssize_t i = 0; i < vector_size(&obj->ary); i++) {
            push_object(env, cells, static_cast<Value *>(vector_get(&obj->ary, i)));
        }
        break;
    case NAT_VALUE_CLASS:
        push_object(env, cells, obj->superclass);
        for (ssize_t i = 0; i < obj->included_modules_count; i++) {
            push_object(env, cells, obj->included_modules[i]);
        }
        if (obj->methods.table) {
            for (iter = hashmap_iter(&obj->methods); iter; iter = hashmap_iter_next(&obj->methods, iter)) {
                Method *method = (Method *)hashmap_iter_get_data(iter);
                if (NAT_OBJ_HAS_ENV(method)) gather_from_env(cells, &method->env);
            }
        }
        break;
    case NAT_VALUE_ENCODING:
        push_object(env, cells, obj->encoding_names);
        break;
    case NAT_VALUE_EXCEPTION:
        push_object(env, cells, obj->backtrace);
        break;
    case NAT_VALUE_FALSE:
        break;
    case NAT_VALUE_HASH:
        if (obj->key_list) {
            key = obj->key_list;
            do {
                push_object(env, cells, key->key);
                push_object(env, cells, key->val);
                key = key->next;
            } while (key != obj->key_list);
        }
        push_object(env, cells, obj->hash_default_value);
        if (obj->hash_default_block && NAT_OBJ_HAS_ENV(obj->hash_default_block)) gather_from_env(cells, &obj->hash_default_block->env);
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
                Method *method = (Method *)hashmap_iter_get_data(iter);
                if (NAT_OBJ_HAS_ENV(method)) gather_from_env(cells, &method->env);
            }
        }
        break;
    case NAT_VALUE_NIL:
        break;
    case NAT_VALUE_OTHER:
        break;
    case NAT_VALUE_PROC:
        gather_from_env(cells, &obj->block->env);
        break;
    case NAT_VALUE_RANGE:
        push_object(env, cells, obj->range_begin);
        push_object(env, cells, obj->range_end);
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

static void mark_live_cells(Env *env) {
    Vector *cells = gather_roots(env);

    for (ssize_t o = 0; o < vector_size(cells); o++) {
        HeapCell *cell = static_cast<HeapCell *>(vector_get(cells, o));

        if (cell->mark) continue;

        cell->mark = true;

        // TODO: branch based on type somehow
        trace_object(env, cells, static_cast<Value *>(NAT_HEAP_OBJ_FROM_CELL(cell)));
    }

    vector_free(cells);
}

static void destroy_hash_key_list(Value *obj) {
    if (obj->key_list) {
        HashKey *key = obj->key_list;
        do {
            HashKey *next_key = key->next;
            free(key);
            key = next_key;
        } while (key != obj->key_list);
    }
}

static bool free_object(Env *env, Value *obj) {
    if (NAT_TYPE(obj) == NAT_VALUE_SYMBOL) return false;

    if (obj->constants.table) hashmap_destroy(&obj->constants);
    if (obj->ivars.table) hashmap_destroy(&obj->ivars);
    struct hashmap_iter *iter;
    switch (obj->type) {
    case NAT_VALUE_ARRAY:
        free(obj->ary.data);
        break;
    case NAT_VALUE_CLASS:
        free(obj->class_name);
        if (obj->methods.table) {
            for (iter = hashmap_iter(&obj->methods); iter; iter = hashmap_iter_next(&obj->methods, iter)) {
                Method *method = (Method *)hashmap_iter_get_data(iter);
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
        destroy_hash_key_list(obj);
        for (iter = hashmap_iter(&obj->hashmap); iter; iter = hashmap_iter_next(&obj->hashmap, iter)) {
            free((HashVal *)hashmap_iter_get_data(iter));
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
                Method *method = (Method *)hashmap_iter_get_data(iter);
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

    return true;
}

static HeapCell *find_sibling_heap_cell(HeapCell *first_cell, HeapCell *cell) {
    HeapCell *sibling_cell = first_cell;
    while (sibling_cell) {
        if (sibling_cell->next == cell) {
            return sibling_cell;
        }
        sibling_cell = sibling_cell->next;
    }
    return NULL;
}

static void collect_cell(Env *env, HeapBlock *block, HeapCell *cell) {
    void *obj = NAT_HEAP_OBJ_FROM_CELL(cell);

    if (obj == gc_abort_if_collected) {
        printf("%p was collected!\n", obj);
        abort();
    }

    // TODO: branch based on type somehow
    if (!free_object(env, static_cast<Value *>(obj))) return;
    memset(obj, 0, cell->size);

    if (cell == block->used_list) { // first cell in list
        block->used_list = cell->next;
    } else {
        HeapCell *sibling_cell = find_sibling_heap_cell(block->used_list, cell);
        assert(sibling_cell);
        sibling_cell->next = cell->next;
    }

    list_prepend(&block->free_list, cell); // TODO: merge with adjacent cells if possible

    env->global_env->bytes_available += cell->size;
}

static void collect_dead_cells(Env *env) {
    HeapBlock *block = env->global_env->heap;
    while (block) {
        HeapCell *cell = block->used_list;
        while (cell) {
            HeapCell *next_cell = cell->next;
            if (!cell->mark) {
                collect_cell(env, block, cell);
            }
            cell = next_cell;
        }
        block = block->next;
    }
}

void gc_init(Env *env, void *bottom_of_stack) {
    env->global_env->bottom_of_stack = bottom_of_stack;
    env->global_env->gc_enabled = true;
}

HeapBlock *gc_alloc_heap_block(GlobalEnv *global_env) {
    HeapBlock *block = static_cast<HeapBlock *>(calloc(1, NAT_HEAP_BLOCK_SIZE));
    HeapCell *cell = NAT_HEAP_BLOCK_FIRST_CELL(block);
    cell->size = NAT_HEAP_BLOCK_SIZE - NAT_HEAP_BLOCK_HEADER_SIZE - NAT_HEAP_CELL_HEADER_SIZE;
    hashmap_put(global_env->heap_cells, cell, block);
    block->free_list = cell;
    list_prepend(&global_env->heap, block);
    HeapCell *max = (HeapCell *)((char *)block + NAT_HEAP_BLOCK_SIZE);
    if (max > global_env->max_ptr) {
        global_env->max_ptr = max;
    }
    HeapCell *min = (HeapCell *)((char *)block + NAT_HEAP_BLOCK_HEADER_SIZE);
    if (min < global_env->min_ptr) {
        global_env->min_ptr = min;
    }
    ssize_t usable = NAT_HEAP_BLOCK_SIZE - NAT_HEAP_BLOCK_HEADER_SIZE - NAT_HEAP_CELL_HEADER_SIZE;
    global_env->bytes_available += usable;
    global_env->bytes_total += usable;
    return block;
}

Value *gc_malloc(Env *env) {
    ssize_t size = sizeof(Value);
    HeapBlock *block = env->global_env->heap;
    do {
        HeapCell *cell = block->free_list;
        HeapCell *prev_cell = NULL;
        while (cell) {
            if (cell->size >= size) {
                ssize_t remaining = cell->size - size - NAT_HEAP_CELL_HEADER_SIZE;
                if (remaining > NAT_HEAP_MIN_CELL_SIZE) {
                    HeapCell *new_cell = (HeapCell *)((char *)cell + NAT_HEAP_CELL_HEADER_SIZE + size);
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
                list_prepend(&block->used_list, cell);
                return static_cast<Value *>(NAT_HEAP_CELL_START_USABLE(cell));
            }
            prev_cell = cell;
            cell = cell->next;
        }
        block = block->next;
    } while (block);
    gc_alloc_heap_block(env->global_env);
    return gc_malloc(env);
}

void gc_collect(Env *env) {
#ifdef NAT_GC_DISABLE
    return;
#endif
    if (!env->global_env->gc_enabled) return;
    env->global_env->gc_enabled = false;
    unmark_all_cells(env);
    mark_live_cells(env);
    collect_dead_cells(env);
    while (gc_bytes_available_ratio(env) < NAT_HEAP_MIN_AVAIL_AFTER_COLLECTION_RATIO) {
        // still not enough, even after a collection
        gc_alloc_heap_block(env->global_env);
    }
    env->global_env->gc_enabled = true;
}

void gc_collect_all(Env *env) {
    unmark_all_cells(env);
    collect_dead_cells(env);
}

double gc_bytes_available_ratio(Env *env) {
    return (double)env->global_env->bytes_available / (double)env->global_env->bytes_total;
}

Value *alloc(Env *env, Value *klass, enum NatValueType type) {
#ifdef NAT_GC_COLLECT_DEBUG
    gc_collect(env);
#else
    if (gc_bytes_available_ratio(env) < NAT_HEAP_MIN_AVAIL_RATIO) {
        gc_collect(env);
    }
#endif
    Value *obj = gc_malloc(env);
    memset(obj, 0, sizeof(Value));
    obj->klass = klass;
    obj->type = type;
    return obj;
}

}
