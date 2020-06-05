#pragma once

#include <stdlib.h>

#define NAT_VECTOR_GROW_FACTOR 2

namespace Natalie {

struct Vector {
    ssize_t size { 0 };
    ssize_t capacity { 0 };
    void **data { nullptr };
};

Vector *vector(ssize_t);
void vector_init(Vector *, ssize_t);
ssize_t vector_size(Vector *);
ssize_t vector_capacity(Vector *);
void **vector_data(Vector *);
void *vector_get(Vector *, ssize_t);
void vector_set(Vector *, ssize_t, void *);
void vector_free(Vector *);
void vector_copy(Vector *, Vector *);
void vector_push(Vector *, void *);
void vector_add(Vector *, Vector *, Vector *);

}
