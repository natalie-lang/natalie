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
        m_globals = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
        hashmap_init(m_globals, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(m_globals, hashmap_alloc_key_string, free);
        m_symbols = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
        hashmap_init(m_symbols, hashmap_identity, hashmap_compare_identity, 100);
        hashmap_set_key_alloc_funcs(m_symbols, nullptr, nullptr);
    }

    ~GlobalEnv() {
        hashmap_destroy(m_globals);
        hashmap_destroy(m_symbols);
    }

    struct hashmap *globals() {
        return m_globals;
    }

    struct hashmap *symbols() {
        return m_symbols;
    }

    ClassValue *Object() { return m_Object; }
    void set_Object(ClassValue *Object) { m_Object = Object; }

    NilValue *nil_obj() { return m_nil_obj; }
    void set_nil_obj(NilValue *nil_obj) { m_nil_obj = nil_obj; }

    TrueValue *true_obj() { return m_true_obj; }
    void set_true_obj(TrueValue *true_obj) { m_true_obj = true_obj; }

    FalseValue *false_obj() { return m_false_obj; }
    void set_false_obj(FalseValue *false_obj) { m_false_obj = false_obj; }

private:
    struct hashmap *m_globals { nullptr };
    struct hashmap *m_symbols { nullptr };
    ClassValue *m_Object { nullptr };
    NilValue *m_nil_obj { nullptr };
    TrueValue *m_true_obj { nullptr };
    FalseValue *m_false_obj { nullptr };
};

}
