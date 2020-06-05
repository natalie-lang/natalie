#include <stdlib.h>

#include "natalie.hpp"

namespace Natalie {

Vector *vector(ssize_t capacity) {
    Vector *vec = static_cast<Vector *>(malloc(sizeof(Vector)));
    vector_init(vec, capacity);
    return vec;
}

void vector_init(Vector *vec, ssize_t capacity) {
    vec->size = 0;
    vec->capacity = capacity;
    if (capacity > 0) {
        vec->data = static_cast<void **>(calloc(capacity, sizeof(void *)));
    } else {
        vec->data = NULL;
    }
}

ssize_t vector_size(Vector *vec) {
    if (!vec) return 0;
    return vec->size;
}

ssize_t vector_capacity(Vector *vec) {
    if (!vec) return 0;
    return vec->capacity;
}

void **vector_data(Vector *vec) {
    return vec->data;
}

void *vector_get(Vector *vec, ssize_t index) {
    return vec->data[index];
}

void vector_set(Vector *vec, ssize_t index, void *item) {
    assert(index < vec->capacity);
    vec->size = NAT_MAX(vec->size, index + 1);
    vec->data[index] = item;
}

void vector_free(Vector *vec) {
    free(vec->data);
    free(vec);
}

void vector_copy(Vector *dest, Vector *source) {
    vector_init(dest, source->capacity);
    dest->size = source->size;
    memcpy(dest->data, source->data, source->size * sizeof(void *));
}

static void vector_grow(Vector *vec, ssize_t capacity) {
    vec->data = static_cast<void **>(realloc(vec->data, sizeof(void *) * capacity));
    vec->capacity = capacity;
}

static void vector_grow_at_least(Vector *vec, ssize_t min_capacity) {
    ssize_t capacity = vec->capacity;
    if (capacity >= min_capacity) {
        return;
    }
    if (capacity > 0 && min_capacity <= capacity * NAT_VECTOR_GROW_FACTOR) {
        vector_grow(vec, capacity * NAT_VECTOR_GROW_FACTOR);
    } else {
        vector_grow(vec, min_capacity);
    }
}

void vector_push(Vector *vec, void *item) {
    ssize_t len = vec->size;
    if (vec->size >= vec->capacity) {
        vector_grow_at_least(vec, len + 1);
    }
    vec->size++;
    vec->data[len] = item;
}

void vector_add(Vector *new_vec, Vector *vec1, Vector *vec2) {
    vector_grow_at_least(new_vec, vec1->size + vec2->size);
    memcpy(new_vec->data, vec1->data, vec1->size * sizeof(void *));
    memcpy(new_vec->data + vec1->size, vec2->data, vec2->size * sizeof(void *));
    new_vec->size = vec1->size + vec2->size;
}

}
