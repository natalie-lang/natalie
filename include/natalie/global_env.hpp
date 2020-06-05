#pragma once

#include <stdlib.h>

#include "natalie/forward.hpp"

namespace Natalie {

extern "C" {
#include "hashmap.h"
#include "onigmo.h"
}

struct GlobalEnv {
    GlobalEnv() {
        globals = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
        hashmap_init(globals, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(globals, hashmap_alloc_key_string, free);
        symbols = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
        hashmap_init(symbols, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(symbols, hashmap_alloc_key_string, free);
    }

    ~GlobalEnv() {
        hashmap_destroy(globals);
        hashmap_destroy(symbols);
    }

    struct hashmap *globals { nullptr };
    struct hashmap *symbols { nullptr };
    ClassValue *Object { nullptr };
    ClassValue *Integer { nullptr };
    NilValue *nil { nullptr };
    TrueValue *true_obj { nullptr };
    FalseValue *false_obj { nullptr };
};

}
